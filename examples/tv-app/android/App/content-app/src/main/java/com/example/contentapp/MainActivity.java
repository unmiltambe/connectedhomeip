package com.example.contentapp;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.util.Base64;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import com.example.contentapp.matter.MatterAgentClient;
import com.matter.tv.app.api.Clusters;
import com.matter.tv.app.api.MatterIntentConstants;
import com.matter.tv.app.api.SetSupportedClustersRequest;
import com.matter.tv.app.api.SupportedCluster;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.nio.charset.StandardCharsets;

public class MainActivity extends AppCompatActivity {

  private static final String TAG = "ContentAppMainActivity";
  private static final String ATTR_PS_PLAYING = "Playback State : PLAYING";
  private static final String ATTR_PS_PAUSED = "Playback State : PAUSED";
  private static final String ATTR_PS_NOT_PLAYING = "Playback State : NOT_PLAYING";
  private static final String ATTR_PS_BUFFERING = "Playback State : BUFFERING";
  private static final String ATTR_TL_LONG_BAD = "Target List : LONG BAD";
  private static final String ATTR_TL_LONG = "Target List : LONG";
  private static final String ATTR_TL_SHORT = "Target List : SHORT";
  private final ExecutorService executorService = Executors.newSingleThreadExecutor();
  private String setupPIN = "";
  private BroadcastReceiver contentReceiver = new BroadcastReceiver() {
    @Override
    public void onReceive(Context context, Intent intent) {
      String action = intent.getAction();
      if ("com.example.contentapp.LAUNCH_VIDEO".equals(action)) {
        String videoUrl = intent.getStringExtra("video_url");
        launchVideoPlayer(videoUrl);
      } else if ("com.example.contentapp.LAUNCH_IMAGE_GALLERY".equals(action)) {
        String galleryUrl = intent.getStringExtra("gallery_url");
        launchImageGallery(galleryUrl);
      }
    }
  };

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    MatterAgentClient.initialize(getApplicationContext());

    setContentView(R.layout.activity_main);

    Intent intent = getIntent();
    String command = intent.getStringExtra(MatterIntentConstants.EXTRA_COMMAND_PAYLOAD);

    // use the text in a TextView
    TextView textView = (TextView) findViewById(R.id.commandTextView);
    textView.setText("Command Payload : " + command);

    Button setupPINButton = findViewById(R.id.setupPINButton);
    if (!setupPIN.isEmpty()) {
      EditText pinText = findViewById(R.id.setupPINText);
      pinText.setText(setupPIN);
    }
    setupPINButton.setOnClickListener(
        view -> {
          EditText pinText = findViewById(R.id.setupPINText);
          String pinStr = pinText.getText().toString();
          setupPIN = pinStr;
          CommandResponseHolder.getInstance()
              .setResponseValue(
                  Clusters.AccountLogin.Id,
                  Clusters.AccountLogin.Commands.GetSetupPIN.ID,
                  "{\""
                      + Clusters.AccountLogin.Commands.GetSetupPINResponse.Fields.SetupPIN
                      + "\":\""
                      + pinStr
                      + "\"}");
        });

    Button attributeUpdateButton = findViewById(R.id.updateAttributeButton);

    attributeUpdateButton.setOnClickListener(
        view -> {
          Spinner dropdown = findViewById(R.id.spinnerAttribute);
          String attribute = (String) dropdown.getSelectedItem();
          switch (attribute) {
            case ATTR_PS_PLAYING:
              AttributeHolder.getInstance()
                  .setAttributeValue(
                      Clusters.MediaPlayback.Id,
                      Clusters.MediaPlayback.Attributes.CurrentState,
                      Clusters.MediaPlayback.Types.PlaybackStateEnum.Playing);
              reportAttributeChange(
                  Clusters.MediaPlayback.Id, Clusters.MediaPlayback.Attributes.CurrentState);
              break;
            case ATTR_PS_PAUSED:
              AttributeHolder.getInstance()
                  .setAttributeValue(
                      Clusters.MediaPlayback.Id,
                      Clusters.MediaPlayback.Attributes.CurrentState,
                      Clusters.MediaPlayback.Types.PlaybackStateEnum.Paused);
              reportAttributeChange(
                  Clusters.MediaPlayback.Id, Clusters.MediaPlayback.Attributes.CurrentState);
              break;
            case ATTR_PS_BUFFERING:
              AttributeHolder.getInstance()
                  .setAttributeValue(
                      Clusters.MediaPlayback.Id,
                      Clusters.MediaPlayback.Attributes.CurrentState,
                      Clusters.MediaPlayback.Types.PlaybackStateEnum.Buffering);
              reportAttributeChange(
                  Clusters.MediaPlayback.Id, Clusters.MediaPlayback.Attributes.CurrentState);
              break;
            case ATTR_PS_NOT_PLAYING:
              AttributeHolder.getInstance()
                  .setAttributeValue(
                      Clusters.MediaPlayback.Id,
                      Clusters.MediaPlayback.Attributes.CurrentState,
                      Clusters.MediaPlayback.Types.PlaybackStateEnum.NotPlaying);
              reportAttributeChange(
                  Clusters.MediaPlayback.Id, Clusters.MediaPlayback.Attributes.CurrentState);
              break;
            case ATTR_TL_LONG_BAD:
              AttributeHolder.getInstance()
                  .setAttributeValue(
                      Clusters.TargetNavigator.Id,
                      Clusters.TargetNavigator.Attributes.TargetList,
                      AttributeHolder.TL_LONG_BAD);
              reportAttributeChange(
                  Clusters.TargetNavigator.Id, Clusters.TargetNavigator.Attributes.TargetList);
              break;
            case ATTR_TL_LONG:
              AttributeHolder.getInstance()
                  .setAttributeValue(
                      Clusters.TargetNavigator.Id,
                      Clusters.TargetNavigator.Attributes.TargetList,
                      AttributeHolder.TL_LONG);
              reportAttributeChange(
                  Clusters.TargetNavigator.Id, Clusters.TargetNavigator.Attributes.TargetList);
              break;
            case ATTR_TL_SHORT:
              AttributeHolder.getInstance()
                  .setAttributeValue(
                      Clusters.TargetNavigator.Id,
                      Clusters.TargetNavigator.Attributes.TargetList,
                      AttributeHolder.TL_SHORT);
              reportAttributeChange(
                  Clusters.TargetNavigator.Id, Clusters.TargetNavigator.Attributes.TargetList);
              break;
          }
        });
    Spinner dropdown = findViewById(R.id.spinnerAttribute);
    String[] items =
        new String[] {
          ATTR_PS_PLAYING,
          ATTR_PS_PAUSED,
          ATTR_PS_NOT_PLAYING,
          ATTR_PS_BUFFERING,
          ATTR_TL_LONG,
          ATTR_TL_SHORT,
          ATTR_TL_LONG_BAD
        };
    ArrayAdapter<String> adapter =
        new ArrayAdapter<>(this, android.R.layout.simple_spinner_dropdown_item, items);
    dropdown.setAdapter(adapter);

    MatterAgentClient matterAgentClient = MatterAgentClient.getInstance();
    if (matterAgentClient != null) {
      SetSupportedClustersRequest supportedClustersRequest = new SetSupportedClustersRequest();
      supportedClustersRequest.supportedClusters = new ArrayList<SupportedCluster>();
      SupportedCluster supportedCluster = new SupportedCluster();
      supportedCluster.clusterIdentifier = 1;
      supportedClustersRequest.supportedClusters.add(supportedCluster);
      executorService.execute(() -> matterAgentClient.reportClusters(supportedClustersRequest));
    }

    // Play Video Button
    Button playVideoButton = findViewById(R.id.playRegularVideoButton);
    playVideoButton.setOnClickListener(v -> {
      String videoUrl = "https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4";
      Intent playVideoIntent = new Intent(MainActivity.this, VideoPlayerActivity.class);
      playVideoIntent.putExtra("video_url", videoUrl);
      startActivity(playVideoIntent);
    });

    // Play DRM Video Button
    Button playDrmButton = findViewById(R.id.playDrmVideoButton);
    playDrmButton.setOnClickListener(v -> {
        try {
          JSONObject drmData = new JSONObject();
          drmData.put("contentUrl", "https://storage.googleapis.com/wvmedia/cenc/h264/tears/tears.mpd");
          drmData.put("licenseServerUrl", "https://proxy.uat.widevine.com/proxy?provider=widevine_test");
          drmData.put("drmScheme", "widevine");

          JSONObject headers = new JSONObject();
          headers.put("Authorization", "Bearer sample-token");
          headers.put("x-playback-token", "abc123");
          headers.put("x-session-id", "session456");
          headers.put("x-request-id", "request789");
          headers.put("x-drm-key-id", "key-xyz");

          drmData.put("httpHeaders", headers);

//          drmData.put("contentUrl", "https://dtkya1w875897.cloudfront.net/da6dc30a-e52f-4af2-9751-000b89416a4e/assets/357577a1-3b61-43ae-9af5-82b9727e2f22/videokit-720p-dash-hls-drm/dash/index.mpd");
//          drmData.put("licenseServerUrl", "https://insys-marketing.la.drm.cloud/acquire-license/widevine");
//          drmData.put("drmScheme", "widevine");
//          drmData.put("userAgent", "ExoPlayer-Drm");
//
//          JSONObject headers = new JSONObject();
//          headers.put("x-drm-brandGuid", "da6dc30a-e52f-4af2-9751-000b89416a4e");
//          headers.put("x-drm-userToken", "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE4OTM0NTYwMDAsImRybVRva2VuSW5mbyI6eyJleHAiOiIyMDMwLTAxLTAxVDAwOjAwOjAwKzAwOjAwIiwia2lkIjpbIjFmODNhZTdmLTMwYzgtNGFkMC04MTcxLTI5NjZhMDFiNjU0NyJdLCJwIjp7InBlcnMiOmZhbHNlfX19.hElVqrfK-iLeV_ZleJJO8i-Mf1D2yYVXdtgBE0ja9R4");

          drmData.put("httpHeaders", headers);

          String drmJson = drmData.toString();
          String base64Encoded = Base64.encodeToString(
                  drmJson.getBytes(StandardCharsets.UTF_8),
                  Base64.NO_WRAP
          );

          Intent drmVideoIntent = new Intent(MainActivity.this, VideoPlayerActivity.class);
          drmVideoIntent.putExtra("video_url", base64Encoded);
          startActivity(drmVideoIntent);

        } catch (JSONException e) {
          e.printStackTrace();
        }
      });

    // View Images Button
    Button viewImagesButton = findViewById(R.id.viewImagesButton);
    viewImagesButton.setOnClickListener(v -> {
      String galleryUrl = "https://picsum.photos/v2/list?page=1&limit=20";
      Intent viewImagesIntent = new Intent(MainActivity.this, ImageViewerActivity.class);
      viewImagesIntent.putExtra("gallery_url", galleryUrl);
      startActivity(viewImagesIntent);
    });

    // Register broadcast receiver
    IntentFilter filter = new IntentFilter();
    filter.addAction("com.example.contentapp.LAUNCH_VIDEO");
    filter.addAction("com.example.contentapp.LAUNCH_IMAGE_GALLERY");
    registerReceiver(contentReceiver, filter);
  }

  private void launchVideoPlayer(String videoUrl) {
    if (videoUrl != null) {
      // Launch video from activity context
      Intent videoIntent = new Intent(MainActivity.this, VideoPlayerActivity.class);
      videoIntent.putExtra("video_url", videoUrl);
      startActivity(videoIntent);
    }
  }

  private void launchImageGallery(String galleryUrl) {
    Intent viewImagesIntent = new Intent(MainActivity.this, ImageViewerActivity.class);
    viewImagesIntent.putExtra("gallery_url", galleryUrl);
    startActivity(viewImagesIntent);
  }

  private void reportAttributeChange(final int clusterId, final int attributeId) {
    executorService.execute(
        new Runnable() {
          @Override
          public void run() {
            MatterAgentClient client = MatterAgentClient.getInstance();
            client.reportAttributeChange(clusterId, attributeId);
          }
        });
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    if (contentReceiver != null) {
      unregisterReceiver(contentReceiver);
    }
  }
}
