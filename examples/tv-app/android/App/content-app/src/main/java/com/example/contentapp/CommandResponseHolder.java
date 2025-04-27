package com.example.contentapp;

import android.content.Context;
import android.content.Intent;
import android.app.PendingIntent;
import android.util.Log;
import android.widget.Toast;
import com.matter.tv.app.api.Clusters;
import java.util.HashMap;
import java.util.Map;

import org.json.JSONException;
import org.json.JSONObject;

/** Class to hold attribute values to help test attribute read and subscribe use cases. */
public class CommandResponseHolder {
  private Map<Long, Map<Long, String>> responseValues = new HashMap<>();
  private static final String TAG = "CommandResponseHolder";
  private static final Long DEFAULT_COMMAND = -1L;
  private static final long CONTENT_LAUNCHER_CLUSTER_ID = 1290;
  private static final long LAUNCH_URL_COMMAND_ID = 1;

  private static CommandResponseHolder instance = new CommandResponseHolder();

  private CommandResponseHolder() {
    // Setting up responses
    setResponseValue(
            Clusters.ContentLauncher.Id,
            DEFAULT_COMMAND,
            "{\"0\":0, \"1\":\"custom response from content app for content launcher\"}");
    setResponseValue(
            Clusters.TargetNavigator.Id,
            DEFAULT_COMMAND,
            "{\"0\":0, \"1\":\"custom response from content app for target navigator\"}");
    setResponseValue(
            Clusters.MediaPlayback.Id,
            DEFAULT_COMMAND,
            "{\"0\":0, \"1\":\"custom response from content app for media playback\"}");
    setResponseValue(
            Clusters.AccountLogin.Id,
            Clusters.AccountLogin.Commands.GetSetupPIN.ID,
            "{\"0\":\"20202021\"}");
    setResponseValue(
            Clusters.AccountLogin.Id,
            Clusters.AccountLogin.Commands.Login.ID,
            // 0 is for success, you can return 1 for failure
            "{\"Status\":0}");
    setResponseValue(
            Clusters.AccountLogin.Id,
            Clusters.AccountLogin.Commands.Logout.ID,
            // 0 is for success, you can return 1 for failure
            "{\"Status\":0}");
  }

  ;

  public static CommandResponseHolder getInstance() {
    return instance;
  }

  public void setResponseValue(long clusterId, long commandId, String value) {
    if (value == null) {
      Log.d(TAG, "Setting null for cluster " + clusterId + " command " + commandId);
    }
    Map<Long, String> responses = responseValues.get(clusterId);
    if (responses == null) {
      responses = new HashMap<>();
      responseValues.put(clusterId, responses);
    }
    responses.put(commandId, value);
  }

  public String getCommandResponse(Context context, long clusterId, long commandId,
                                   String command) {
    Log.d(TAG, "Getting response for clusterId " + clusterId +
            " commandId " + commandId + " command " + command);

    if (CONTENT_LAUNCHER_CLUSTER_ID == clusterId) {
        Log.d(TAG, "Received ContentLauncher command " + commandId);
        if (LAUNCH_URL_COMMAND_ID == commandId) {
          Log.d(TAG, "Calling LaunchURL");
          launchURL(context, command);
        } else {
          Log.e(TAG, "Ignoring unhandled ContentLauncher commandId " + commandId +
              " command " + command);
        }
    }

    // Return one of the pre-determined responses
    Map<Long, String> responses = responseValues.get(clusterId);
    String response = responses.get(commandId);
    if (response == null) {
      response = responses.get(DEFAULT_COMMAND);
    }
    return response;
  }

  private void launchURL(Context context, String jsonCommand) {
    Log.d(TAG, "In LaunchURL");
    try {
      if (context == null) {
        throw new IllegalStateException("Context not set. Call setContext first.");
      }

      Context appContext = context.getApplicationContext();

      if (jsonCommand == null || jsonCommand.isEmpty()) {
        throw new JSONException("Empty response");
      }

      JSONObject jsonObject = new JSONObject(jsonCommand);
      if (!jsonObject.has("0")) {
        throw new JSONException("No URL found in response");
      }

      String videoUrl = jsonObject.getString("0");
      if (!videoUrl.startsWith("http://") && !videoUrl.startsWith("https://")) {
        throw new JSONException("Invalid URL format");
      }

      if (jsonObject.has("1")) {
        String displayString = jsonObject.getString("1");
        if (!displayString.isEmpty()) {
          Toast.makeText(appContext, "Playing " + displayString + "\n" + videoUrl,
                  Toast.LENGTH_LONG).show();
        }
      }

      Log.d(TAG, "Sending intent to MainActivity to play video");
      Intent intent = new Intent("com.example.contentapp.LAUNCH_VIDEO");
      intent.putExtra("video_url", videoUrl);
      context.sendBroadcast(intent);
      Log.d(TAG, "Broadcast sent to MainActivity");

    } catch (Exception e) {
      e.printStackTrace();
      Toast.makeText(context, "Error: " + e.getMessage(), Toast.LENGTH_SHORT).show();
    }
  }
}