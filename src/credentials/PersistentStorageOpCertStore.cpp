/*
 *    Copyright (c) 2022 Project CHIP Authors
 *    All rights reserved.
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

#include <string.h>

#include <crypto/CHIPCryptoPAL.h>
#include <lib/core/CHIPError.h>
#include <lib/core/CHIPPersistentStorageDelegate.h>
#include <lib/core/DataModelTypes.h>
#include <lib/core/TLV.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/DefaultStorageKeyAllocator.h>
#include <lib/support/SafeInt.h>
#include <lib/support/ScopedBuffer.h>

#include <credentials/CHIPCert.h>

#include "PersistentStorageOpCertStore.h"

namespace chip {
namespace Credentials {

namespace {

using CertChainElement       = OperationalCertificateStore::CertChainElement;
using VidVerificationElement = OperationalCertificateStore::VidVerificationElement;

StorageKeyName GetStorageKeyForCert(FabricIndex fabricIndex, CertChainElement element)
{
    switch (element)
    {
    case CertChainElement::kNoc:
        return DefaultStorageKeyAllocator::FabricNOC(fabricIndex);
        break;
    case CertChainElement::kIcac:
        return DefaultStorageKeyAllocator::FabricICAC(fabricIndex);
        break;
    case CertChainElement::kRcac:
        return DefaultStorageKeyAllocator::FabricRCAC(fabricIndex);
        break;
    default:
        break;
    }

    return StorageKeyName::Uninitialized();
}

bool StorageHasCertificate(PersistentStorageDelegate * storage, FabricIndex fabricIndex, CertChainElement element)
{
    StorageKeyName storageKey = GetStorageKeyForCert(fabricIndex, element);

    if (!storageKey)
    {
        return false;
    }

    // TODO(#16958): need to actually read the cert to know if it's there due to platforms not
    //               properly enforcing CHIP_ERROR_BUFFER_TOO_SMALL behavior needed by
    //               PersistentStorageDelegate.
    uint8_t placeHolderCertBuffer[kMaxCHIPCertLength];

    uint16_t keySize = sizeof(placeHolderCertBuffer);
    CHIP_ERROR err   = storage->SyncGetKeyValue(storageKey.KeyName(), &placeHolderCertBuffer[0], keySize);

    return (err == CHIP_NO_ERROR);
}

CHIP_ERROR LoadCertFromStorage(PersistentStorageDelegate * storage, FabricIndex fabricIndex, CertChainElement element,
                               MutableByteSpan & outCert)
{
    StorageKeyName storageKey = GetStorageKeyForCert(fabricIndex, element);
    if (!storageKey)
    {
        return CHIP_ERROR_INTERNAL;
    }

    uint16_t keySize = static_cast<uint16_t>(outCert.size());
    CHIP_ERROR err   = storage->SyncGetKeyValue(storageKey.KeyName(), outCert.data(), keySize);

    // Not finding an ICAC means we don't have one, so adjust to meet the API contract, where
    // outCert.empty() will be true;
    if ((element == CertChainElement::kIcac) && (err == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND))
    {
        outCert.reduce_size(0);
        return CHIP_ERROR_NOT_FOUND;
    }

    if (err == CHIP_NO_ERROR)
    {
        outCert.reduce_size(keySize);
    }
    else if (err == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND)
    {
        // Convert persisted storage error to CHIP_ERROR_NOT_FOUND so that
        // `PersistentStorageOpCertStore::GetCertificate` doesn't need to convert.
        err = CHIP_ERROR_NOT_FOUND;
    }

    return err;
}

CHIP_ERROR SaveCertToStorage(PersistentStorageDelegate * storage, FabricIndex fabricIndex, CertChainElement element,
                             const ByteSpan & cert)
{
    StorageKeyName storageKey = GetStorageKeyForCert(fabricIndex, element);
    if (!storageKey)
    {
        return CHIP_ERROR_INTERNAL;
    }

    // If provided an empty ICAC, we delete the ICAC key previously used. If not there, it's OK
    if ((element == CertChainElement::kIcac) && (cert.empty()))
    {
        CHIP_ERROR err = storage->SyncDeleteKeyValue(storageKey.KeyName());
        if ((err == CHIP_NO_ERROR) || (err == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND))
        {
            return CHIP_NO_ERROR;
        }
        return err;
    }

    return storage->SyncSetKeyValue(storageKey.KeyName(), cert.data(), static_cast<uint16_t>(cert.size()));
}

CHIP_ERROR DeleteCertFromStorage(PersistentStorageDelegate * storage, FabricIndex fabricIndex, CertChainElement element)
{
    StorageKeyName storageKey = GetStorageKeyForCert(fabricIndex, element);
    if (!storageKey)
    {
        return CHIP_ERROR_INTERNAL;
    }
    return storage->SyncDeleteKeyValue(storageKey.KeyName());
}

CHIP_ERROR SaveVidVerificationElementToStorage(PersistentStorageDelegate * storage, FabricIndex fabricIndex,
                                               VidVerificationElement element, ByteSpan elementData)
{
    StorageKeyName storageKey = StorageKeyName::FromConst("");

    switch (element)
    {
    case VidVerificationElement::kVidVerificationStatement:
        storageKey = DefaultStorageKeyAllocator::FabricVidVerificationStatement(fabricIndex);
        break;
    case VidVerificationElement::kVvsc:
        storageKey = DefaultStorageKeyAllocator::FabricVVSC(fabricIndex);
        break;
    default:
        return CHIP_ERROR_INTERNAL;
    }

    if (!storageKey)
    {
        return CHIP_ERROR_INTERNAL;
    }

    if (elementData.empty())
    {
        CHIP_ERROR err = storage->SyncDeleteKeyValue(storageKey.KeyName());
        if ((err == CHIP_NO_ERROR) || (err == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND))
        {
            return CHIP_NO_ERROR;
        }
        return err;
    }

    return storage->SyncSetKeyValue(storageKey.KeyName(), elementData.data(), static_cast<uint16_t>(elementData.size()));
}

CHIP_ERROR DeleteVidVerificationElementFromStorage(PersistentStorageDelegate * storage, FabricIndex fabricIndex,
                                                   VidVerificationElement element)
{
    // Saving an empty bytespan actually deletes the element.
    return SaveVidVerificationElementToStorage(storage, fabricIndex, element, ByteSpan{});
}

} // namespace

bool PersistentStorageOpCertStore::HasPendingRootCert() const
{
    if (mStorage == nullptr)
    {
        return false;
    }

    return (mPendingRcac.Get() != nullptr) && mStateFlags.Has(StateFlags::kAddNewTrustedRootCalled);
}

bool PersistentStorageOpCertStore::HasPendingNocChain() const
{
    if (mStorage == nullptr)
    {
        return false;
    }

    return (mPendingNoc.Get() != nullptr) && mStateFlags.HasAny(StateFlags::kAddNewOpCertsCalled, StateFlags::kUpdateOpCertsCalled);
}

bool PersistentStorageOpCertStore::HasCertificateForFabric(FabricIndex fabricIndex, CertChainElement element) const
{
    if ((mStorage == nullptr) || !IsValidFabricIndex(fabricIndex))
    {
        return false;
    }

    // FabricIndex matches pending, we MAY have some pending data
    if (fabricIndex == mPendingFabricIndex)
    {
        switch (element)
        {
        case CertChainElement::kRcac:
            if (mPendingRcac.Get() != nullptr)
            {
                return true;
            }
            break;
        case CertChainElement::kIcac:
            if (mPendingIcac.Get() != nullptr)
            {
                return true;
            }
            // If we have a pending NOC and no pending ICAC, don't delegate to storage, return not found here
            // since in the pending state, there truly is nothing.
            if (mPendingNoc.Get() != nullptr)
            {
                return false;
            }
            break;
        case CertChainElement::kNoc:
            if (mPendingNoc.Get() != nullptr)
            {
                return true;
            }
            break;
        default:
            return false;
        }
    }

    return StorageHasCertificate(mStorage, fabricIndex, element);
}

bool PersistentStorageOpCertStore::HasNocChainForFabric(FabricIndex fabricIndex) const
{
    // If we have at least RCAC and NOC, we are good. Chain may be invalid without ICAC, but caller is to ensure that.
    return (HasCertificateForFabric(fabricIndex, CertChainElement::kRcac) &&
            HasCertificateForFabric(fabricIndex, CertChainElement::kNoc));
}

bool PersistentStorageOpCertStore::HasPendingVidVerificationElements() const
{
    // If any VID verifications statement data has been touched, we may need to store or erase data on commit.
    return mStateFlags.HasAny(StateFlags::kVidVerificationStatementUpdated, StateFlags::kVvscUpdated);
}

CHIP_ERROR PersistentStorageOpCertStore::AddNewTrustedRootCertForFabric(FabricIndex fabricIndex, const ByteSpan & rcac)
{
    VerifyOrReturnError(mStorage != nullptr, CHIP_ERROR_INCORRECT_STATE);
    VerifyOrReturnError(IsValidFabricIndex(fabricIndex), CHIP_ERROR_INVALID_FABRIC_INDEX);
    VerifyOrReturnError(!rcac.empty() && (rcac.size() <= Credentials::kMaxCHIPCertLength), CHIP_ERROR_INVALID_ARGUMENT);

    VerifyOrReturnError(!mStateFlags.HasAny(StateFlags::kUpdateOpCertsCalled, StateFlags::kAddNewTrustedRootCalled,
                                            StateFlags::kAddNewOpCertsCalled),
                        CHIP_ERROR_INCORRECT_STATE);
    VerifyOrReturnError(!StorageHasCertificate(mStorage, fabricIndex, CertChainElement::kRcac), CHIP_ERROR_INCORRECT_STATE);

    Platform::ScopedMemoryBufferWithSize<uint8_t> rcacBuf;
    VerifyOrReturnError(rcacBuf.Alloc(rcac.size()), CHIP_ERROR_NO_MEMORY);
    memcpy(rcacBuf.Get(), rcac.data(), rcac.size());

    mPendingRcac = std::move(rcacBuf);

    mPendingFabricIndex = fabricIndex;
    mStateFlags.Set(StateFlags::kAddNewTrustedRootCalled);

    return CHIP_NO_ERROR;
}

CHIP_ERROR PersistentStorageOpCertStore::AddNewOpCertsForFabric(FabricIndex fabricIndex, const ByteSpan & noc,
                                                                const ByteSpan & icac)
{
    VerifyOrReturnError(mStorage != nullptr, CHIP_ERROR_INCORRECT_STATE);
    VerifyOrReturnError(IsValidFabricIndex(fabricIndex), CHIP_ERROR_INVALID_FABRIC_INDEX);
    VerifyOrReturnError(!noc.empty() && (noc.size() <= Credentials::kMaxCHIPCertLength), CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(icac.size() <= Credentials::kMaxCHIPCertLength, CHIP_ERROR_INVALID_ARGUMENT);
    // Can't have called UpdateOpCertsForFabric first, or called with pending certs
    VerifyOrReturnError(!mStateFlags.HasAny(StateFlags::kUpdateOpCertsCalled, StateFlags::kAddNewOpCertsCalled),
                        CHIP_ERROR_INCORRECT_STATE);

    // Need to have trusted roots installed to make the chain valid
    VerifyOrReturnError(mStateFlags.Has(StateFlags::kAddNewTrustedRootCalled), CHIP_ERROR_INCORRECT_STATE);

    // fabricIndex must match the current pending fabric
    VerifyOrReturnError(fabricIndex == mPendingFabricIndex, CHIP_ERROR_INVALID_FABRIC_INDEX);

    // Can't have persisted NOC/ICAC for same fabric if adding
    VerifyOrReturnError(!StorageHasCertificate(mStorage, fabricIndex, CertChainElement::kNoc), CHIP_ERROR_INCORRECT_STATE);
    VerifyOrReturnError(!StorageHasCertificate(mStorage, fabricIndex, CertChainElement::kIcac), CHIP_ERROR_INCORRECT_STATE);

    Platform::ScopedMemoryBufferWithSize<uint8_t> nocBuf;
    VerifyOrReturnError(nocBuf.Alloc(noc.size()), CHIP_ERROR_NO_MEMORY);
    memcpy(nocBuf.Get(), noc.data(), noc.size());

    Platform::ScopedMemoryBufferWithSize<uint8_t> icacBuf;
    if (icac.size() > 0)
    {
        VerifyOrReturnError(icacBuf.Alloc(icac.size()), CHIP_ERROR_NO_MEMORY);
        memcpy(icacBuf.Get(), icac.data(), icac.size());
    }

    mPendingNoc  = std::move(nocBuf);
    mPendingIcac = std::move(icacBuf);

    mStateFlags.Set(StateFlags::kAddNewOpCertsCalled);

    return CHIP_NO_ERROR;
}

CHIP_ERROR PersistentStorageOpCertStore::UpdateOpCertsForFabric(FabricIndex fabricIndex, const ByteSpan & noc,
                                                                const ByteSpan & icac)
{
    VerifyOrReturnError(mStorage != nullptr, CHIP_ERROR_INCORRECT_STATE);
    VerifyOrReturnError(IsValidFabricIndex(fabricIndex), CHIP_ERROR_INVALID_FABRIC_INDEX);
    VerifyOrReturnError(!noc.empty() && (noc.size() <= Credentials::kMaxCHIPCertLength), CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(icac.size() <= Credentials::kMaxCHIPCertLength, CHIP_ERROR_INVALID_ARGUMENT);

    // Can't have called AddNewOpCertsForFabric first, and should never get here after AddNewTrustedRootCertForFabric.
    VerifyOrReturnError(!mStateFlags.HasAny(StateFlags::kAddNewOpCertsCalled, StateFlags::kAddNewTrustedRootCalled),
                        CHIP_ERROR_INCORRECT_STATE);

    // Can't have already pending NOC from UpdateOpCerts not yet committed
    VerifyOrReturnError(!mStateFlags.Has(StateFlags::kUpdateOpCertsCalled), CHIP_ERROR_INCORRECT_STATE);

    // Need to have trusted roots installed to make the chain valid
    VerifyOrReturnError(StorageHasCertificate(mStorage, fabricIndex, CertChainElement::kRcac), CHIP_ERROR_INCORRECT_STATE);

    // Must have persisted NOC for same fabric if updating
    VerifyOrReturnError(StorageHasCertificate(mStorage, fabricIndex, CertChainElement::kNoc), CHIP_ERROR_INCORRECT_STATE);

    // Don't check for ICAC, we may not have had one before, but assume that if NOC is there, a
    // previous chain was at least partially there

    Platform::ScopedMemoryBufferWithSize<uint8_t> nocBuf;
    VerifyOrReturnError(nocBuf.Alloc(noc.size()), CHIP_ERROR_NO_MEMORY);
    memcpy(nocBuf.Get(), noc.data(), noc.size());

    Platform::ScopedMemoryBufferWithSize<uint8_t> icacBuf;
    if (icac.size() > 0)
    {
        VerifyOrReturnError(icacBuf.Alloc(icac.size()), CHIP_ERROR_NO_MEMORY);
        memcpy(icacBuf.Get(), icac.data(), icac.size());
    }

    mPendingNoc  = std::move(nocBuf);
    mPendingIcac = std::move(icacBuf);

    // For NOC update, UpdateOpCertsForFabric is what determines the pending fabric index,
    // not a previous AddNewTrustedRootCertForFabric call.
    mPendingFabricIndex = fabricIndex;

    mStateFlags.Set(StateFlags::kUpdateOpCertsCalled);

    return CHIP_NO_ERROR;
}

CHIP_ERROR PersistentStorageOpCertStore::BasicVidVerificationAssumptionsAreMet(FabricIndex fabricIndex) const
{
    VerifyOrReturnError(mStorage != nullptr, CHIP_ERROR_INCORRECT_STATE);
    VerifyOrReturnError(IsValidFabricIndex(fabricIndex), CHIP_ERROR_INVALID_FABRIC_INDEX);
    // Must already have a valid NOC chain.
    VerifyOrReturnError(HasNocChainForFabric(fabricIndex), CHIP_ERROR_INCORRECT_STATE);

    return CHIP_NO_ERROR;
}

CHIP_ERROR PersistentStorageOpCertStore::UpdateVidVerificationSignerCertForFabric(FabricIndex fabricIndex, ByteSpan vvsc)
{
    ReturnErrorOnFailure(BasicVidVerificationAssumptionsAreMet(fabricIndex));
    VerifyOrReturnError(vvsc.empty() || vvsc.size() <= Credentials::kMaxCHIPCertLength, CHIP_ERROR_INVALID_ARGUMENT);

    CHIP_ERROR vvscErr = CHIP_NO_ERROR;

    if (vvsc.empty())
    {
        if (fabricIndex == mPendingFabricIndex)
        {
            mPendingVvsc.Free();
            mStateFlags.Set(StateFlags::kVvscUpdated);
        }
        else
        {
            vvscErr = DeleteVidVerificationElementFromStorage(mStorage, fabricIndex, VidVerificationElement::kVvsc);
        }
    }
    else
    {
        if (fabricIndex == mPendingFabricIndex)
        {
            VerifyOrReturnError(mPendingVvsc.Alloc(vvsc.size()), CHIP_ERROR_NO_MEMORY);
            memcpy(mPendingVvsc.Get(), vvsc.data(), vvsc.size());
            mStateFlags.Set(StateFlags::kVvscUpdated);
        }
        else
        {
            vvscErr = SaveVidVerificationElementToStorage(mStorage, fabricIndex, VidVerificationElement::kVvsc, vvsc);
        }
    }

    return vvscErr;
}

CHIP_ERROR PersistentStorageOpCertStore::UpdateVidVerificationStatementForFabric(FabricIndex fabricIndex,
                                                                                 ByteSpan vidVerificationStatement)
{
    ReturnErrorOnFailure(BasicVidVerificationAssumptionsAreMet(fabricIndex));
    VerifyOrReturnError(vidVerificationStatement.empty() ||
                            vidVerificationStatement.size() == Crypto::kVendorIdVerificationStatementV1Size,
                        CHIP_ERROR_INVALID_ARGUMENT);

    CHIP_ERROR vvsErr = CHIP_NO_ERROR;

    if (vidVerificationStatement.empty())
    {
        if (fabricIndex == mPendingFabricIndex)
        {
            mPendingVidVerificationStatement.Free();
            mStateFlags.Set(StateFlags::kVidVerificationStatementUpdated);
        }
        else
        {
            vvsErr =
                DeleteVidVerificationElementFromStorage(mStorage, fabricIndex, VidVerificationElement::kVidVerificationStatement);
        }
    }
    else
    {
        if (fabricIndex == mPendingFabricIndex)
        {
            VerifyOrReturnError(mPendingVidVerificationStatement.Alloc(vidVerificationStatement.size()), CHIP_ERROR_NO_MEMORY);
            memcpy(mPendingVidVerificationStatement.Get(), vidVerificationStatement.data(), vidVerificationStatement.size());
            mStateFlags.Set(StateFlags::kVidVerificationStatementUpdated);
        }
        else
        {
            vvsErr = SaveVidVerificationElementToStorage(mStorage, fabricIndex, VidVerificationElement::kVidVerificationStatement,
                                                         vidVerificationStatement);
        }
    }

    return vvsErr;
}

CHIP_ERROR PersistentStorageOpCertStore::CommitOpCertsForFabric(FabricIndex fabricIndex)
{
    VerifyOrReturnError(mStorage != nullptr, CHIP_ERROR_INCORRECT_STATE);
    VerifyOrReturnError(IsValidFabricIndex(fabricIndex) && (fabricIndex == mPendingFabricIndex), CHIP_ERROR_INVALID_FABRIC_INDEX);

    VerifyOrReturnError(HasPendingNocChain(), CHIP_ERROR_INCORRECT_STATE);
    if (HasPendingRootCert())
    {
        // Neither of these conditions should have occurred based on other interlocks, but since
        // committing certificates is a dangerous operation, we absolutely validate our assumptions.
        VerifyOrReturnError(!mStateFlags.Has(StateFlags::kUpdateOpCertsCalled), CHIP_ERROR_INCORRECT_STATE);
        VerifyOrReturnError(mStateFlags.Has(StateFlags::kAddNewTrustedRootCalled), CHIP_ERROR_INCORRECT_STATE);
    }

    // TODO: Handle transaction marking to revert partial certs at next boot if we get interrupted by reboot.

    // Start committing NOC first so we don't have dangling roots if one was added.
    ByteSpan pendingNocSpan{ mPendingNoc.Get(), mPendingNoc.AllocatedSize() };
    CHIP_ERROR nocErr = SaveCertToStorage(mStorage, mPendingFabricIndex, CertChainElement::kNoc, pendingNocSpan);

    // ICAC storage handles deleting on empty/missing
    ByteSpan pendingIcacSpan{ mPendingIcac.Get(), mPendingIcac.AllocatedSize() };
    CHIP_ERROR icacErr = SaveCertToStorage(mStorage, mPendingFabricIndex, CertChainElement::kIcac, pendingIcacSpan);

    CHIP_ERROR rcacErr = CHIP_NO_ERROR;
    if (HasPendingRootCert())
    {
        ByteSpan pendingRcacSpan{ mPendingRcac.Get(), mPendingRcac.AllocatedSize() };
        rcacErr = SaveCertToStorage(mStorage, mPendingFabricIndex, CertChainElement::kRcac, pendingRcacSpan);
    }

    CHIP_ERROR vidVerifyErr = CommitVidVerificationForFabric(mPendingFabricIndex);

    // Remember which was the first error, and if any error occurred.
    CHIP_ERROR stickyErr = nocErr;
    stickyErr            = (stickyErr != CHIP_NO_ERROR) ? stickyErr : icacErr;
    stickyErr            = (stickyErr != CHIP_NO_ERROR) ? stickyErr : rcacErr;
    stickyErr            = (stickyErr != CHIP_NO_ERROR) ? stickyErr : vidVerifyErr;

    if (stickyErr != CHIP_NO_ERROR)
    {
        // On Adds rather than updates, remove anything possibly stored for the new fabric on partial
        // failure.
        if (mStateFlags.Has(StateFlags::kAddNewOpCertsCalled))
        {
            (void) DeleteCertFromStorage(mStorage, mPendingFabricIndex, CertChainElement::kNoc);
            (void) DeleteCertFromStorage(mStorage, mPendingFabricIndex, CertChainElement::kIcac);
            (void) DeleteVidVerificationElementFromStorage(mStorage, mPendingFabricIndex, VidVerificationElement::kVvsc);
            (void) DeleteVidVerificationElementFromStorage(mStorage, mPendingFabricIndex,
                                                           VidVerificationElement::kVidVerificationStatement);
        }
        if (mStateFlags.Has(StateFlags::kAddNewTrustedRootCalled))
        {
            (void) DeleteCertFromStorage(mStorage, mPendingFabricIndex, CertChainElement::kRcac);
        }
        if (mStateFlags.Has(StateFlags::kUpdateOpCertsCalled))
        {
            // Can't do anything to clean-up here, but pretty sure the fabric is broken now...
            // TODO: Handle transaction marking to revert certs if somehow failing store on update by pre-backing-up opcerts
        }

        return stickyErr;
    }

    // If we got here, we succeeded and can reset the pending certs: next `GetCertificate` will use the stored certs
    RevertPendingOpCerts();
    return CHIP_NO_ERROR;
}

bool PersistentStorageOpCertStore::HasAnyCertificateForFabric(FabricIndex fabricIndex) const
{
    VerifyOrReturnError(IsValidFabricIndex(fabricIndex), false);

    bool rcacMissing = !StorageHasCertificate(mStorage, fabricIndex, CertChainElement::kRcac);
    bool icacMissing = !StorageHasCertificate(mStorage, fabricIndex, CertChainElement::kIcac);
    bool nocMissing  = !StorageHasCertificate(mStorage, fabricIndex, CertChainElement::kNoc);
    bool anyPending  = (mPendingRcac.Get() != nullptr) || (mPendingIcac.Get() != nullptr) || (mPendingNoc.Get() != nullptr);

    if (rcacMissing && icacMissing && nocMissing && !anyPending)
    {
        return false;
    }

    return true;
}

CHIP_ERROR PersistentStorageOpCertStore::RemoveOpCertsForFabric(FabricIndex fabricIndex)
{
    VerifyOrReturnError(mStorage != nullptr, CHIP_ERROR_INCORRECT_STATE);
    VerifyOrReturnError(IsValidFabricIndex(fabricIndex), CHIP_ERROR_INVALID_FABRIC_INDEX);

    // If there was *no* state, pending or persisted, we have an error
    VerifyOrReturnError(HasAnyCertificateForFabric(fabricIndex), CHIP_ERROR_INVALID_FABRIC_INDEX);

    // Clear any pending state
    RevertPendingOpCerts();

    // Remove all persisted certs for the given fabric, blindly
    CHIP_ERROR nocErr  = DeleteCertFromStorage(mStorage, fabricIndex, CertChainElement::kNoc);
    CHIP_ERROR icacErr = DeleteCertFromStorage(mStorage, fabricIndex, CertChainElement::kIcac);
    CHIP_ERROR rcacErr = DeleteCertFromStorage(mStorage, fabricIndex, CertChainElement::kRcac);

    CHIP_ERROR vvscErr = DeleteVidVerificationElementFromStorage(mStorage, fabricIndex, VidVerificationElement::kVvsc);
    CHIP_ERROR vvsErr =
        DeleteVidVerificationElementFromStorage(mStorage, fabricIndex, VidVerificationElement::kVidVerificationStatement);

    // Ignore missing data errors
    nocErr  = (nocErr == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND) ? CHIP_NO_ERROR : nocErr;
    icacErr = (icacErr == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND) ? CHIP_NO_ERROR : icacErr;
    rcacErr = (rcacErr == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND) ? CHIP_NO_ERROR : rcacErr;
    vvscErr = (vvscErr == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND) ? CHIP_NO_ERROR : vvscErr;
    vvsErr  = (vvsErr == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND) ? CHIP_NO_ERROR : vvsErr;

    // Find the first error and return that
    CHIP_ERROR stickyErr = nocErr;
    stickyErr            = (stickyErr != CHIP_NO_ERROR) ? stickyErr : icacErr;
    stickyErr            = (stickyErr != CHIP_NO_ERROR) ? stickyErr : rcacErr;
    stickyErr            = (stickyErr != CHIP_NO_ERROR) ? stickyErr : vvscErr;
    stickyErr            = (stickyErr != CHIP_NO_ERROR) ? stickyErr : vvsErr;

    return stickyErr;
}

CHIP_ERROR PersistentStorageOpCertStore::CommitVidVerificationForFabric(FabricIndex fabricIndex)
{
    VerifyOrReturnError(mStorage != nullptr, CHIP_ERROR_INCORRECT_STATE);
    if (!HasPendingVidVerificationElements())
    {
        return CHIP_NO_ERROR;
    }

    VerifyOrReturnError(IsValidFabricIndex(fabricIndex) && (fabricIndex == mPendingFabricIndex), CHIP_ERROR_INVALID_FABRIC_INDEX);

    CHIP_ERROR vvscErr = CHIP_NO_ERROR;
    CHIP_ERROR vvsErr  = CHIP_NO_ERROR;

    if (mStateFlags.Has(StateFlags::kVvscUpdated))
    {
        ByteSpan pendingVvscSpan{ mPendingVvsc.Get(), mPendingVvsc.AllocatedSize() };
        vvscErr =
            SaveVidVerificationElementToStorage(mStorage, mPendingFabricIndex, VidVerificationElement::kVvsc, pendingVvscSpan);
    }

    if (mStateFlags.Has(StateFlags::kVidVerificationStatementUpdated))
    {
        ByteSpan pendingVidVerificationStatementSpan{ mPendingVidVerificationStatement.Get(),
                                                      mPendingVidVerificationStatement.AllocatedSize() };
        vvsErr = SaveVidVerificationElementToStorage(
            mStorage, mPendingFabricIndex, VidVerificationElement::kVidVerificationStatement, pendingVidVerificationStatementSpan);
    }

    // Remember which was the first error, and if any error occurred.
    CHIP_ERROR stickyErr = vvscErr;
    stickyErr            = (stickyErr != CHIP_NO_ERROR) ? stickyErr : vvsErr;

    return stickyErr;
}

CHIP_ERROR PersistentStorageOpCertStore::GetPendingCertificate(FabricIndex fabricIndex, CertChainElement element,
                                                               MutableByteSpan & outCertificate) const
{
    if (fabricIndex != mPendingFabricIndex)
    {
        return CHIP_ERROR_NOT_FOUND;
    }

    // FabricIndex matches pending, we MAY have some pending data
    switch (element)
    {
    case CertChainElement::kRcac:
        if (mPendingRcac.Get() != nullptr)
        {
            ByteSpan rcacSpan{ mPendingRcac.Get(), mPendingRcac.AllocatedSize() };
            return CopySpanToMutableSpan(rcacSpan, outCertificate);
        }
        break;
    case CertChainElement::kIcac:
        if (mPendingIcac.Get() != nullptr)
        {
            ByteSpan icacSpan{ mPendingIcac.Get(), mPendingIcac.AllocatedSize() };
            return CopySpanToMutableSpan(icacSpan, outCertificate);
        }
        break;
    case CertChainElement::kNoc:
        if (mPendingNoc.Get() != nullptr)
        {
            ByteSpan nocSpan{ mPendingNoc.Get(), mPendingNoc.AllocatedSize() };
            return CopySpanToMutableSpan(nocSpan, outCertificate);
        }
        break;
    default:
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    return CHIP_ERROR_NOT_FOUND;
}

CHIP_ERROR PersistentStorageOpCertStore::GetCertificate(FabricIndex fabricIndex, CertChainElement element,
                                                        MutableByteSpan & outCertificate) const
{
    VerifyOrReturnError(mStorage != nullptr, CHIP_ERROR_INCORRECT_STATE);
    VerifyOrReturnError(IsValidFabricIndex(fabricIndex), CHIP_ERROR_INVALID_FABRIC_INDEX);

    // Handle case of pending data
    CHIP_ERROR err = GetPendingCertificate(fabricIndex, element, outCertificate);
    if ((err == CHIP_NO_ERROR) || (err != CHIP_ERROR_NOT_FOUND))
    {
        // Found in pending, or got a deeper error: return the pending cert status.
        return err;
    }

    // If we have a pending NOC and no pending ICAC, don't delegate to storage, return not found here
    // since in the pending state, there truly is nothing.

    if ((err == CHIP_ERROR_NOT_FOUND) && (element == CertChainElement::kIcac) && (mPendingNoc.Get() != nullptr))
    {
        // Don't delegate to storage if we just have a pending NOC and are missing the ICAC
        return CHIP_ERROR_NOT_FOUND;
    }

    // Not found in pending, let's look in persisted
    return LoadCertFromStorage(mStorage, fabricIndex, element, outCertificate);
}

CHIP_ERROR PersistentStorageOpCertStore::GetVidVerificationElement(FabricIndex fabricIndex, VidVerificationElement element,
                                                                   MutableByteSpan & outElement) const
{
    VerifyOrReturnError(mStorage != nullptr, CHIP_ERROR_INCORRECT_STATE);
    VerifyOrReturnError(IsValidFabricIndex(fabricIndex), CHIP_ERROR_INVALID_FABRIC_INDEX);

    StorageKeyName keyName = StorageKeyName::FromConst("");
    if (element == VidVerificationElement::kVidVerificationStatement)
    {
        if (mStateFlags.Has(StateFlags::kVidVerificationStatementUpdated) && (fabricIndex == mPendingFabricIndex))
        {
            return CopySpanToMutableSpan(
                ByteSpan{ mPendingVidVerificationStatement.Get(), mPendingVidVerificationStatement.AllocatedSize() }, outElement);
        }

        keyName = DefaultStorageKeyAllocator::FabricVidVerificationStatement(fabricIndex);
    }

    if (element == VidVerificationElement::kVvsc)
    {
        if (mStateFlags.Has(StateFlags::kVvscUpdated) && (fabricIndex == mPendingFabricIndex))
        {
            return CopySpanToMutableSpan(ByteSpan{ mPendingVvsc.Get(), mPendingVvsc.AllocatedSize() }, outElement);
        }

        keyName = DefaultStorageKeyAllocator::FabricVVSC(fabricIndex);
    }

    if (!keyName)
    {
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    uint8_t storageBuffer[kMaxCHIPCertLength];
    uint16_t keySize = sizeof(storageBuffer);
    static_assert(kMaxCHIPCertLength > (2 * (Crypto::kVendorIdVerificationStatementV1Size)),
                  "Assuming that at least two VidVerificationStatement fit in a CHIP Cert to give space for future growth and "
                  "upgrade/downgrade scenarios.");

    CHIP_ERROR err = mStorage->SyncGetKeyValue(keyName.KeyName(), &storageBuffer[0], keySize);
    if ((err == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND) || (err == CHIP_ERROR_NOT_FOUND))
    {
        outElement.reduce_size(0);
        return CHIP_NO_ERROR;
    }

    if (err == CHIP_NO_ERROR)
    {
        return CopySpanToMutableSpan(ByteSpan{ &storageBuffer[0], static_cast<size_t>(keySize) }, outElement);
    }

    return err;
}

} // namespace Credentials
} // namespace chip
