/*
 *
 *    Copyright (c) 2020-2022 Project CHIP Authors
 *    Copyright 2023-2025 NXP
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "FactoryDataProviderImpl.h"

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

#include "ELSFactoryData.h"
#include "mflash_drv.h"

#if defined(MBEDTLS_THREADING_C) && defined(MBEDTLS_THREADING_ALT)
#include "els_pkc_mbedtls.h"
#endif /* defined(MBEDTLS_THREADING_C) && defined(MBEDTLS_THREADING_ALT) */

#include "fsl_adapter_flash.h"

/* mbedtls */
#include "mbedtls/aes.h"
#include "mbedtls/sha256.h"

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#ifndef FACTORY_DATA_PROVIDER_LOG
#define FACTORY_DATA_PROVIDER_LOG 0
#endif

#if FACTORY_DATA_PROVIDER_LOG
#include "fsl_debug_console.h"
#define FACTORY_DATA_PROVIDER_PRINTF(...)                                                                                          \
    PRINTF("[%s] ", __FUNCTION__);                                                                                                 \
    PRINTF(__VA_ARGS__);                                                                                                           \
    PRINTF("\n\r");
#else
#define FACTORY_DATA_PROVIDER_PRINTF(...)
#endif

#if CONFIG_CHIP_OTA_FACTORY_DATA_PROCESSOR
#error("OTA FACTORY DATA PROCESSOR NOT SUPPORTED WITH THIS FACTORY DATA PRVD IMPL")
#endif

/* Grab symbol for the base address from the linker file. */
extern uint32_t __FACTORY_DATA_START_OFFSET[];
extern uint32_t __FACTORY_DATA_SIZE[];

using namespace ::chip::Credentials;
using namespace ::chip::Crypto;

namespace chip {
namespace DeviceLayer {

FactoryDataProviderImpl FactoryDataProviderImpl::sInstance;

static constexpr size_t kPrivateKeyBlobLength = Crypto::kP256_PrivateKey_Length + ELS_BLOB_METADATA_SIZE + ELS_WRAP_OVERHEAD;

CHIP_ERROR FactoryDataProviderImpl::DecryptAesEcb(uint8_t * dest, uint8_t * source)
{
    uint8_t res = 0;
    mbedtls_aes_context aesCtx;

    mbedtls_aes_init(&aesCtx);
    res = mbedtls_aes_setkey_dec(&aesCtx, pAesKey, pAESKeySize);
    VerifyOrReturnError(res == 0, CHIP_ERROR_INTERNAL);

    res = mbedtls_aes_crypt_ecb(&aesCtx, MBEDTLS_AES_DECRYPT, source, dest);
    VerifyOrReturnError(res == 0, CHIP_ERROR_INTERNAL);

    mbedtls_aes_free(&aesCtx);

    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProviderImpl::SearchForId(uint8_t searchedType, uint8_t * pBuf, size_t bufLength, uint16_t & length,
                                                uint32_t * contentAddr)
{
    CHIP_ERROR err               = CHIP_ERROR_NOT_FOUND;
    uint8_t type                 = 0;
    uint32_t index               = 0;
    uint8_t * addrContent        = NULL;
    uint8_t * factoryDataAddress = &factoryDataRamBuffer[0];
    uint32_t factoryDataSize     = sizeof(factoryDataRamBuffer);
    uint16_t currentLen          = 0;

    while (index < factoryDataSize)
    {
        /* Read the type */
        memcpy((uint8_t *) &type, factoryDataAddress + index, sizeof(type));
        index += sizeof(type);

        /* Read the len */
        memcpy((uint8_t *) &currentLen, factoryDataAddress + index, sizeof(currentLen));
        index += sizeof(currentLen);

        /* Check if the type gotten is the expected one */
        if (searchedType == type)
        {
            FACTORY_DATA_PROVIDER_PRINTF("type = %d, currentLen = %d, bufLength =%d", type, currentLen, bufLength);
            /* If pBuf is null it means that we only want to know if the Type has been found */
            if (pBuf != NULL)
            {
                /* If the buffer given is too small, fill only the available space */
                if (bufLength < currentLen)
                {
                    currentLen = bufLength;
                }
                memcpy((uint8_t *) pBuf, factoryDataAddress + index, currentLen);
            }
            length = currentLen;
            if (contentAddr != NULL)
            {
                *contentAddr = (uint32_t) factoryDataAddress + index;
            }
            err = CHIP_NO_ERROR;
            break;
        }
        else if (type == 0)
        {
            /* No more type available , break the loop */
            break;
        }
        else
        {
            /* Jump to next data */
            index += currentLen;
        }
    }

    return err;
}

CHIP_ERROR FactoryDataProviderImpl::SignWithDacKey(const ByteSpan & digestToSign, MutableByteSpan & outSignBuffer)
{
    CHIP_ERROR res = CHIP_NO_ERROR;
    uint8_t els_key_blob[kPrivateKeyBlobLength];
    size_t els_key_blob_size = sizeof(els_key_blob);
    uint16_t keySize         = 0;
    status_t status          = STATUS_SUCCESS;
    uint8_t digest[kSHA256_Hash_Length];

    mcuxClEls_KeyIndex_t key_index           = MCUXCLELS_KEY_SLOTS;
    mcuxClEls_KeyProp_t plain_key_properties = {
        .word = { .value = MCUXCLELS_KEYPROPERTY_VALUE_SECURE | MCUXCLELS_KEYPROPERTY_VALUE_PRIVILEGED |
                      MCUXCLELS_KEYPROPERTY_VALUE_KEY_SIZE_256 | MCUXCLELS_KEYPROPERTY_VALUE_KGSRC }
    };

    mcuxClEls_EccSignOption_t sign_options = { 0 };
    mcuxClEls_EccByte_t ecc_signature[MCUXCLELS_ECC_SIGNATURE_SIZE];

    uint8_t public_key[64] = { 0 };
    size_t public_key_size = sizeof(public_key);

    /* Search key ID FactoryDataId::kDacPrivateKeyId */
    ReturnErrorOnFailure(SearchForId(FactoryDataId::kDacPrivateKeyId, els_key_blob, els_key_blob_size, keySize));

    PLOG_DEBUG_BUFFER("els_key_blob", els_key_blob, els_key_blob_size);

    /* Calculate message HASH to sign */
    memset(&digest[0], 0, sizeof(digest));
    res = Hash_SHA256(digestToSign.data(), digestToSign.size(), &digest[0]);
    if (res != CHIP_NO_ERROR)
    {
        return res;
    }

    PLOG_DEBUG_BUFFER("digestToSign", digestToSign.data(), digestToSign.size());

#if defined(MBEDTLS_THREADING_C) && defined(MBEDTLS_THREADING_ALT)
    (void) mcux_els_mutex_lock();
#endif
    /* Import blob DAC key into SE50 (reserved key slot) */
    status = import_die_int_wrapped_key_into_els(els_key_blob, els_key_blob_size, plain_key_properties, &key_index);
    STATUS_SUCCESS_OR_EXIT_MSG("import_die_int_wrapped_key_into_els failed: 0x%08x", status);

    /* For ECC keys that were created from plain key material, there is the
     neceessity to convert them to a key. Converting to a key also yields the public key.
     The conversion can be done either before re-wrapping (when importing the plain key)
     or after (when importing the blob).*/
    status = els_keygen(key_index, &public_key[0], &public_key_size);
    STATUS_SUCCESS_OR_EXIT_MSG("els_keygen failed: 0x%08x", status);

    /* The key is usable for signing. */
    PLOG_DEBUG_BUFFER("public_key", public_key, public_key_size);

    /* ECC sign message hash with the key index slot reserved during the blob importation */
    ELS_sign_hash(digest, ecc_signature, &sign_options, key_index);

    /* Delete SE50 key with the index slot reserved during the blob importation (free key slot) */
    els_delete_key(key_index);

    /* Generate MutableByteSpan with ECC signature and ECC signature size */
#if defined(MBEDTLS_THREADING_C) && defined(MBEDTLS_THREADING_ALT)
    (void) mcux_els_mutex_unlock();
#endif
    return CopySpanToMutableSpan(ByteSpan{ ecc_signature, MCUXCLELS_ECC_SIGNATURE_SIZE }, outSignBuffer);

exit:
    els_delete_key(key_index);
#if defined(MBEDTLS_THREADING_C) && defined(MBEDTLS_THREADING_ALT)
    (void) mcux_els_mutex_unlock();
#endif
    return CHIP_ERROR_INVALID_SIGNATURE;
}

CHIP_ERROR FactoryDataProviderImpl::ReadAndCheckFactoryDataInFlash(void)
{
    status_t status;
    uint32_t factoryDataAddress = (uint32_t) __FACTORY_DATA_START_OFFSET;
    uint32_t factoryDataSize    = (uint32_t) __FACTORY_DATA_SIZE;
    uint32_t hashId;
    uint8_t calculatedHash[SHA256_OUTPUT_SIZE];
    CHIP_ERROR res;
    uint8_t currentBlock[16];

    /* Init mflash */
    status = mflash_drv_init();

    if (status != kStatus_Success || factoryDataSize > sizeof(factoryDataRamBuffer))
        return CHIP_ERROR_INTERNAL;

    /* Load the factory data into RAM buffer */
    if (mflash_drv_read(factoryDataAddress, (uint32_t *) &factoryDataRamBuffer[0], factoryDataSize) != kStatus_Success)
    {
        return CHIP_ERROR_INTERNAL;
    }
    memcpy(&mHeader, factoryDataRamBuffer, sizeof(mHeader));
    if (mHeader.hashId != HASH_ID)
    {
        return CHIP_ERROR_NOT_FOUND;
    }
    /* remove the header section */
    memmove(&factoryDataRamBuffer[0], &factoryDataRamBuffer[sizeof(mHeader)], mHeader.size);

    /* Calculate SHA256 value over the factory data and compare with stored value */
    res = Hash_SHA256(&factoryDataRamBuffer[0], mHeader.size, &calculatedHash[0]);

    if (res != CHIP_NO_ERROR)
        return res;

    if (memcmp(&calculatedHash[0], &mHeader.hash[0], HASH_LEN) != 0)
    {
        /* HASH value didn't match, test if factory data are encrypted */

        /* try to decrypt factory data, reset factory data buffer content*/
        memset(factoryDataRamBuffer, 0, sizeof(factoryDataRamBuffer));
        memset(calculatedHash, 0, sizeof(calculatedHash));

        factoryDataAddress += sizeof(Header);

        /* Load the buffer into RAM by reading each 16 bytes blocks */
        for (int i = 0; i < (mHeader.size / 16); i++)
        {
            if (mflash_drv_read(factoryDataAddress + i * 16, (uint32_t *) &currentBlock[0], sizeof(currentBlock)) !=
                kStatus_Success)
            {
                return CHIP_ERROR_INTERNAL;
            }
            ReturnErrorOnFailure(DecryptAesEcb(&factoryDataRamBuffer[i * 16], &currentBlock[0]));
        }

        /* Calculate SHA256 value over the factory data and compare with stored value */
        res = Hash_SHA256(&factoryDataRamBuffer[0], mHeader.size, &calculatedHash[0]);
        if (memcmp(&calculatedHash[0], &mHeader.hash[0], HASH_LEN) != 0)
        {
            return CHIP_ERROR_NOT_FOUND;
        }
    }

    ChipLogProgress(DeviceLayer, "factory data hash check is successful!");
    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProviderImpl::SetEncryptionMode(EncryptionMode mode)
{
    CHIP_ERROR error = CHIP_ERROR_INVALID_ARGUMENT;

    /*
     * Currently the fwk_factory_data_provider module supports only ecb mode.
     * Therefore return an error if encrypt mode is not ecb
     */
    if (mode == encrypt_ecb)
    {
        encryptMode = mode;
        error       = CHIP_NO_ERROR;
    }
    return error;
}

CHIP_ERROR FactoryDataProviderImpl::Init(void)
{
    uint16_t len;
    uint8_t type;
    uint16_t keySize = 0;

    ReturnLogErrorOnFailure(ReadAndCheckFactoryDataInFlash());

    els_enable();

    ChipLogProgress(DeviceLayer, "init: only protect DAC private key\n");

    /* check whether the kDacPrivateKeyId data is converted or not*/
    ReturnErrorOnFailure(SearchForId(FactoryDataId::kDacPrivateKeyId, NULL, 0, keySize));
    if (keySize == kPrivateKeyBlobLength)
    {
        /* the kDacPrivateKeyId data is converted already, do nothing */
        ChipLogProgress(DeviceLayer, "SSS: DAC private key already converted to blob");
        return CHIP_NO_ERROR;
    }
    else
    {
        /* provison the dac private key into Edge Lock and the returned wrapped key is stored the previous area of factory data,
         update the hash and re-write the factory data in Flash */
        ChipLogProgress(DeviceLayer, "SSS: convert DAC private key to blob");
        ReturnLogErrorOnFailure(ELS_ConvertDacKey());
        ChipLogProgress(DeviceLayer, "System restarting");
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProviderImpl::ELS_ConvertDacKey()
{
    size_t blobSize                     = kPrivateKeyBlobLength;
    size_t newSize                      = sizeof(Header) + mHeader.size + (ELS_BLOB_METADATA_SIZE + ELS_WRAP_OVERHEAD);
    uint8_t blob[kPrivateKeyBlobLength] = { 0 };
    uint32_t KeyAddr;
    uint32_t factoryDataAddress = (uint32_t) __FACTORY_DATA_START_OFFSET;
    uint32_t factoryDataSize    = (uint32_t) __FACTORY_DATA_SIZE;

    uint8_t * data = static_cast<uint8_t *>(chip::Platform::MemoryAlloc(newSize));
    /* Import pain DAC key and generate the blob */
    ReturnErrorOnFailure(ELS_ExportBlob(blob, &blobSize, KeyAddr));
    ChipLogProgress(DeviceLayer, "SSS: extracted blob from DAC private key");
    PLOG_DEBUG_BUFFER("blob", blob, blobSize);

    /* Read all factory data */
    hal_flash_status_t status = HAL_FlashRead(factoryDataAddress + MFLASH_BASE_ADDRESS, sizeof(Header), data);
    memcpy(data + sizeof(Header), factoryDataRamBuffer, mHeader.size);
    VerifyOrReturnError(status == kStatus_HAL_Flash_Success, CHIP_ERROR_INTERNAL);
    ChipLogError(DeviceLayer, "SSS: cached factory data in RAM");

    /* Replace private plain DAC key by the blob into factory data RAM buffer (the blob length is higher then the plain key length)
     */
    ReturnErrorOnFailure(ReplaceWithBlob(data, blob, blobSize, KeyAddr));
    ChipLogError(DeviceLayer, "SSS: replaced DAC private key with secured blob");

    /* Erase flash factory data sectors */
    status = HAL_FlashEraseSector(factoryDataAddress + MFLASH_BASE_ADDRESS, factoryDataSize);
    VerifyOrReturnError(status == kStatus_HAL_Flash_Success, CHIP_ERROR_INTERNAL);
    /* Write new factory data into flash */
    status = HAL_FlashProgramUnaligned(factoryDataAddress + MFLASH_BASE_ADDRESS, newSize, data);
    VerifyOrReturnError(status == kStatus_HAL_Flash_Success, CHIP_ERROR_INTERNAL);
    ChipLogError(DeviceLayer, "SSS: updated factory data");

    /* remove the header section as it will no longer be used */
    memmove(&data[0], &data[sizeof(mHeader)], newSize);
    memset(factoryDataRamBuffer, 0, sizeof(factoryDataRamBuffer));
    memcpy(factoryDataRamBuffer, data, newSize);
    /* Actualisation of the factory data payload size */
    mHeader.size = newSize;

    chip::Platform::MemoryFree(data);
    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProviderImpl::ELS_ExportBlob(uint8_t * data, size_t * dataLen, uint32_t & addr)
{
    status_t status = STATUS_SUCCESS;
    uint8_t keyBuf[Crypto::kP256_PrivateKey_Length];
    uint16_t keySize = 0;
    MutableByteSpan keySpan(keyBuf);

    /* Search key ID FactoryDataId::kDacPrivateKeyId */
    ReturnErrorOnFailure(SearchForId(FactoryDataId::kDacPrivateKeyId, keySpan.data(), keySpan.size(), keySize, &addr));
    keySpan.reduce_size(keySize);
    PLOG_DEBUG_BUFFER("Private DAC key plain", keySpan.data(), keySpan.size());
    mcuxClEls_KeyProp_t plain_key_properties = {
        .word = { .value = MCUXCLELS_KEYPROPERTY_VALUE_SECURE | MCUXCLELS_KEYPROPERTY_VALUE_PRIVILEGED |
                      MCUXCLELS_KEYPROPERTY_VALUE_KEY_SIZE_256 | MCUXCLELS_KEYPROPERTY_VALUE_KGSRC }
    };

    mcuxClEls_KeyIndex_t key_index = MCUXCLELS_KEY_SLOTS;
    /* Import plain DAC key into S50 */
    status = import_plain_key_into_els(keySpan.data(), keySpan.size(), plain_key_properties, &key_index);
    STATUS_SUCCESS_OR_EXIT_MSG("derive_key failed: 0x%08x", status);

    /* ELS generate key blob. The blob created here is one that can be directly imported into ELS again. */
    status = export_key_from_els(key_index, data, dataLen);
    STATUS_SUCCESS_OR_EXIT_MSG("export_key_from_els failed: 0x%08x", status);

    status = els_delete_key(key_index);
    STATUS_SUCCESS_OR_EXIT_MSG("delete_key failed: 0x%08x", status);
exit:
    return CHIP_NO_ERROR;
}

CHIP_ERROR FactoryDataProviderImpl::ReplaceWithBlob(uint8_t * data, uint8_t * blob, size_t blobLen, uint32_t KeyAddr)
{
    size_t newSize                           = mHeader.size + ELS_BLOB_METADATA_SIZE + ELS_WRAP_OVERHEAD;
    FactoryDataProviderImpl::Header * header = reinterpret_cast<FactoryDataProviderImpl::Header *>(data);
    uint8_t * payload                        = data + sizeof(FactoryDataProviderImpl::Header);
    uint8_t offset                           = (uint8_t *) (KeyAddr - kValueOffset) - (uint8_t *) &factoryDataRamBuffer[0];
    size_t subsequentDataOffset              = offset + kValueOffset + Crypto::kP256_PrivateKey_Length;

    memmove(payload + subsequentDataOffset + ELS_BLOB_METADATA_SIZE + ELS_WRAP_OVERHEAD, payload + subsequentDataOffset,
            mHeader.size - subsequentDataOffset);
    header->size = newSize;
    /* Update associated TLV length */
    memcpy(payload + offset + kLengthOffset, (uint16_t *) &blobLen, sizeof(uint16_t));
    /* Replace private plain DAC key by the blob */
    memcpy(payload + offset + kValueOffset, blob, blobLen);

    /* Update Header with new hash value */
    uint8_t hash[Crypto::kSHA256_Hash_Length] = { 0 };
    ReturnErrorOnFailure(Crypto::Hash_SHA256(payload, header->size, hash));
    memcpy(header->hash, hash, sizeof(header->hash));

    return CHIP_NO_ERROR;
}

FactoryDataProvider & FactoryDataPrvdImpl()
{
    return FactoryDataProviderImpl::sInstance;
}

} // namespace DeviceLayer
} // namespace chip
