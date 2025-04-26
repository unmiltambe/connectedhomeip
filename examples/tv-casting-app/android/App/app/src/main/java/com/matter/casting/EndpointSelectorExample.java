package com.matter.casting;

import android.util.Log;
import com.matter.casting.core.CastingPlayer;
import com.matter.casting.core.Endpoint;
import java.util.List;

/** A utility that selects an endpoint based on some criterion */
public class EndpointSelectorExample {
  private static final String TAG = EndpointSelectorExample.class.getSimpleName();
  private static final Integer SAMPLE_ENDPOINT_VID = 65521;
  private static final Integer DEFAULT_VIDEO_PLAYER_ENDPOINT_ID = 1;

  /**
   * Returns the first Endpoint in the list of Endpoints associated with the selectedCastingPlayer
   * whose VendorID matches the EndpointSelectorExample.SAMPLE_ENDPOINT_VID
   */
  public static Endpoint selectFirstEndpointByVID(CastingPlayer selectedCastingPlayer) {
    Log.d(TAG, "UDBG: In selectFirstEndpointByVID()");
    Endpoint endpoint = null;
    if (selectedCastingPlayer != null) {
      List<Endpoint> endpoints = selectedCastingPlayer.getEndpoints();
      endpoints.forEach(ep -> Log.d(TAG, "UDBG: visible ep: " + ep.getId() + " with vid: " + ep.getVendorId()));
      if (endpoints == null) {
        Log.e(TAG, "selectFirstEndpointByVID() No Endpoints found on CastingPlayer");
      } else {
        endpoint =
            endpoints
                .stream()
                .filter(e -> SAMPLE_ENDPOINT_VID.equals(e.getVendorId()))
                .findFirst()
                .orElse(null);
        if (endpoint != null) {
          Log.d(TAG, "UDBG: In selectFirstEndpointByVID(): Found endpoint " +
              endpoint.getId());
        }
      }
    }
    return endpoint;
  }

  /**
   * Returns the Endpoint with the desired endpoint Id in the list of Endpoints associated with the
   * selectedCastingPlayer.
   */
  public static Endpoint selectEndpointById(
      CastingPlayer selectedCastingPlayer, int desiredEndpointId) {
    Endpoint endpoint = null;
    if (selectedCastingPlayer != null) {
      List<Endpoint> endpoints = selectedCastingPlayer.getEndpoints();
      endpoints.forEach(ep -> Log.d(TAG, "UDBG: visible ep: " + ep.getId() +
              " with vid: " + ep.getVendorId()));
      if (endpoints == null) {
        Log.e(TAG, "selectEndpointById() No Endpoints found on CastingPlayer");
      } else {
        endpoint =
            endpoints.stream().filter(e -> desiredEndpointId == e.getId()).findFirst().orElse(null);
        if (endpoint != null) {
          Log.d(TAG, "UDBG: In selectEndpointById(): Found endpoint " +
                  endpoint.getId());
        }
      }
    }
    return endpoint;
  }

  /**
   * Returns the default video player endpoint on the receiver that can be used for remote control,
   * app-less casting or mirroring.
   *
   * @param selectedCastingPlayer The selected video player.
   * @return The default video player endpoint.
   */
  public static Endpoint getDefaultVideoPlayerEndpoint(CastingPlayer selectedCastingPlayer) {
    Endpoint endpoint = null;
    Log.d(TAG, "UDBG: In getDefaultVideoPlayerEndpoint()");
    if (selectedCastingPlayer != null) {
      endpoint = selectEndpointById(selectedCastingPlayer, DEFAULT_VIDEO_PLAYER_ENDPOINT_ID);
      Log.d(TAG, "UDBG: getDefaultVideoPlayerEndpoint(): Found endpoint " + endpoint.getId());
      // TODO: Fails with runtime error:
      // java.lang.UnsatisfiedLinkError: No implementation found for java.util.List com.matter.casting.core.MatterEndpoint.getDeviceTypeList() (tried Java_com_matter_casting_core_MatterEndpoint_getDeviceTypeList and Java_com_matter_casting_core_MatterEndpoint_getDeviceTypeList__) - is the library loaded, e.g. System.loadLibrary?
      //endpoint.getDeviceTypeList().forEach(dt -> Log.d(TAG, "UDBG: contains deviceType: " + dt.deviceType));
    }
    return endpoint;
  }
}
