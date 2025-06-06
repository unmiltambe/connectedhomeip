package com.example.contentapp;

import android.net.Uri;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Base64;
import android.util.Log;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import com.google.android.exoplayer2.SimpleExoPlayer;
import com.google.android.exoplayer2.MediaItem;
import com.google.android.exoplayer2.Player;
import com.google.android.exoplayer2.ui.PlayerView;
import com.google.android.exoplayer2.source.MediaSource;
import com.google.android.exoplayer2.source.dash.DashMediaSource;
import com.google.android.exoplayer2.source.hls.HlsMediaSource;
import com.google.android.exoplayer2.source.smoothstreaming.SsMediaSource;
import com.google.android.exoplayer2.upstream.DefaultHttpDataSource;
import com.google.android.exoplayer2.upstream.DefaultDataSourceFactory;
import com.google.android.exoplayer2.upstream.HttpDataSource;
import com.google.android.exoplayer2.drm.DrmSessionManager;
import com.google.android.exoplayer2.drm.FrameworkMediaDrm;
import com.google.android.exoplayer2.drm.HttpMediaDrmCallback;
import com.google.android.exoplayer2.drm.DefaultDrmSessionManager;
import com.google.android.exoplayer2.util.Util;
import com.google.gson.Gson;
import com.google.gson.reflect.TypeToken;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;
import java.nio.charset.StandardCharsets;

public class VideoPlayerActivity extends AppCompatActivity {
    private static final String TAG = "VideoPlayerActivity";
    private static final String WIDEVINE_UUID = "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed";
    private static final String PLAYREADY_UUID = "9a04f079-9840-4286-ab92-e65be0885f95";
    private static final String CLEARKEY_UUID = "e2719d58-a985-b3c9-781a-b030af78d30e";
    private static final String FAIRPLAY_UUID = "94ce86fb-07ff-4f43-adb8-93d2fa968ca2"; // Used more in HLS context
    private static final String MARLIN_UUID   = "5e629af5-38da-4063-8977-97ffbd9902d4";
    private static final String USER_AGENT = "ExoPlayer-Drm";
    private PlayerView playerView;
    private SimpleExoPlayer player;
    private final Player.Listener playerListener = new Player.Listener() {
        @Override
        public void onPlayerError(com.google.android.exoplayer2.PlaybackException error) {
            String errorMessage = "An error occurred: " + error.getLocalizedMessage();
            Toast.makeText(VideoPlayerActivity.this, errorMessage, Toast.LENGTH_LONG).show();
        }
    };

    // DRM configuration model class
    private static class DrmConfig {
        private String contentUrl;
        private String licenseServerUrl;
        private String drmScheme;
        private String mimeType;
        private Map<String, String> httpHeaders;
        public String getContentUrl() { return contentUrl; }
        public String getLicenseServerUrl() { return licenseServerUrl; }
        public String getDrmScheme() { return drmScheme; }
        public String getMimeType() { return mimeType; }
        public Map<String, String> getHttpHeaders() { return httpHeaders; }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate");
        setContentView(R.layout.activity_video_player);

        playerView = findViewById(R.id.player_view);

        findViewById(R.id.closeButton).setOnClickListener(v -> {
            Log.d(TAG, "Close button clicked");
            finish();
        });

        // Get video url from intent (may be a regular URL or JSON string)
        String videoUrl = getIntent().getStringExtra("video_url");
        
        if (videoUrl == null) {
            Toast.makeText(this, "No video URL provided", Toast.LENGTH_LONG).show();
            finish();
            return;
        }
        
        Log.d(TAG, "Received video URL");
        
        if (videoUrl.startsWith("http://") || videoUrl.startsWith("https://")) {
            // Regular URL - use standard player
            Log.d(TAG, "Regular video URL detected: " + videoUrl);
            initializePlayer(videoUrl);
        } else {
            // This appears to be DRM Config String encoded as Base64
            try {
                String jsonConfig = base64ToString(videoUrl);
                if (jsonConfig == null || jsonConfig.isEmpty()) {
                    Log.e(TAG, "Decoded base64 string is empty");
                    return;
                }
                DrmConfig drmConfig = parseDrmConfig(jsonConfig);
                if (drmConfig == null) {
                    Log.e(TAG, "Failed to parse JSON config: " + jsonConfig);
                    return;
                }
                initializePlayerWithDrm(drmConfig);
            } catch (Exception e) {
                Log.e(TAG, "Failed to parse DRM configuration: " + e.getMessage(), e);
                Toast.makeText(this, "Failed to parse video configuration: " + e.getMessage(), Toast.LENGTH_LONG).show();
                finish();
            }
        }
    }

    /**
     * Convert Base64 encoded string to JSON
     */
    private String base64ToString(String base64) {
        Log.d(TAG, "Decoding Base64 to string");
        try {
            byte[] decodedBytes = Base64.decode(base64, Base64.DEFAULT);
            String base64String = new String(decodedBytes, StandardCharsets.UTF_8);
            Log.e(TAG, "Decoded string: " + base64String);
            return base64String;
        } catch (IllegalArgumentException e) {
            Log.e(TAG, "Failed to decode base64 to string" + base64);
            return null;
        }
    }

    /**
     * Parse the JSON string into a DrmConfig object
     */
    private DrmConfig parseDrmConfig(String jsonData) throws JSONException {
        Log.i(TAG, "Parsing DRM configuration JSON: " + jsonData);
        if (jsonData == null || !jsonData.trim().startsWith("{")) {
            Log.e(TAG, "Invalid JSON: " + jsonData);
            return null;
        }

        // Convert JSON to config class object
        DrmConfig config = new Gson().fromJson(jsonData, DrmConfig.class);

        // Validate required fields
        if (TextUtils.isEmpty(config.getContentUrl())) {
            throw new IllegalArgumentException("contentUrl is required in DRM configuration");
        }
        if (TextUtils.isEmpty(config.getLicenseServerUrl())) {
            throw new IllegalArgumentException("licenseServerUrl is required in DRM configuration");
        }
        if (TextUtils.isEmpty(config.getDrmScheme())) {
            throw new IllegalArgumentException("drmScheme is required in DRM configuration");
        }

        return config;
    }

    /**
     * Initialize player for regular content
     */
    private void initializePlayer(String videoUrl) {
        Log.d(TAG, "Initializing regular player");

        player = new SimpleExoPlayer.Builder(this).build();
        playerView.setPlayer(player); // Link the PlayerView with ExoPlayer

        // Set the media item
        MediaItem mediaItem = MediaItem.fromUri(videoUrl);
        player.setMediaItem(mediaItem);
        player.prepare();
        player.play();

        player.addListener(playerListener);
    }

    /**
     * Initialize player with DRM configuration
     */
    private void initializePlayerWithDrm(DrmConfig config) {
        Log.d(TAG, "Initializing DRM player");
        try {
            // Create player
            player = new SimpleExoPlayer.Builder(this).build();
            playerView.setPlayer(player);

            // Get content URL and DRM license URL
            String contentUrl = config.getContentUrl();
            String licenseUrl = config.getLicenseServerUrl();
            Log.i(TAG, "Content URL: " + contentUrl);
            Log.i(TAG, "License URL: " + licenseUrl);

            // Get MIME type
            String mimeType = config.getMimeType();
            if (TextUtils.isEmpty(mimeType)) {
                mimeType = "application/dash+xml"; // Default to DASH if not specified
            }
            Log.i(TAG, "MIME type: " + mimeType);

            // HTTP headers
            Map<String, String> httpHeaders = config.getHttpHeaders();
            if (httpHeaders == null) {
                httpHeaders = new HashMap<>();
            }
            
            // Create HTTP data source factory with headers
            HttpDataSource.Factory httpDataSourceFactory = new DefaultHttpDataSource.Factory()
                    .setUserAgent(USER_AGENT)
                    .setDefaultRequestProperties(httpHeaders)
                    .setAllowCrossProtocolRedirects(true);

            // Create DRM callback
            Log.i(TAG, "DRM scheme: " + config.getDrmScheme());
            UUID drmSchemeUuid = getDrmUuid(config.getDrmScheme());
            HttpMediaDrmCallback drmCallback = new HttpMediaDrmCallback(licenseUrl, httpDataSourceFactory);
            
            // Add headers to license request if needed
            for (Map.Entry<String, String> header : httpHeaders.entrySet()) {
                drmCallback.setKeyRequestProperty(header.getKey(), header.getValue());
            }

            // Create DRM session manager
            DrmSessionManager drmSessionManager = new DefaultDrmSessionManager.Builder()
                    .setUuidAndExoMediaDrmProvider(drmSchemeUuid, FrameworkMediaDrm.DEFAULT_PROVIDER)
                    .setMultiSession(false)
                    .build(drmCallback);

            // Create media source based on content type
            MediaSource mediaSource = createMediaSource(contentUrl, mimeType, httpDataSourceFactory, drmSessionManager);
            
            // Prepare player
            player.setMediaSource(mediaSource);
            player.prepare();
            player.play();
            player.addListener(playerListener);
            
        } catch (Exception e) {
            Log.e(TAG, "Error initializing DRM player: " + e.getMessage(), e);
            Toast.makeText(this, "Failed to initialize DRM player: " + e.getMessage(), Toast.LENGTH_LONG).show();
            finish();
        }
    }

    /**
     * Get the DRM UUID based on the scheme name
     */
    private UUID getDrmUuid(String schemeId) {
        switch (schemeId.toLowerCase()) {
            case "widevine":
                return UUID.fromString(WIDEVINE_UUID);
            case "playready":
                return UUID.fromString(PLAYREADY_UUID);
            case "clearkey":
                return UUID.fromString(CLEARKEY_UUID);
            case "fairplay":
                return UUID.fromString(FAIRPLAY_UUID);
            case "marlin":
                return UUID.fromString(MARLIN_UUID);
            default:
                Log.w(TAG, "Unknown DRM scheme: " + schemeId + ", defaulting to Widevine");
                return UUID.fromString(WIDEVINE_UUID);
        }
    }

    /**
     * Create the appropriate media source based on content type
     */
    private MediaSource createMediaSource(String contentUrl, String mimeType, 
                                         HttpDataSource.Factory dataSourceFactory,
                                         DrmSessionManager drmSessionManager) {
        Uri uri = Uri.parse(contentUrl);
        
        if (mimeType.contains("dash")) {
            return new DashMediaSource.Factory(dataSourceFactory)
                    .setDrmSessionManager(drmSessionManager)
                    .createMediaSource(MediaItem.fromUri(uri));
        } else if (mimeType.contains("hls")) {
            return new HlsMediaSource.Factory(dataSourceFactory)
                    .setDrmSessionManager(drmSessionManager)
                    .createMediaSource(MediaItem.fromUri(uri));
        } else if (mimeType.contains("smoothstreaming")) {
            return new SsMediaSource.Factory(dataSourceFactory)
                    .setDrmSessionManager(drmSessionManager)
                    .createMediaSource(MediaItem.fromUri(uri));
        } else {
            // Default to progressive media source for other types
            Log.w(TAG, "Unrecognized MIME type: " + mimeType + ", using DASH");
            return new DashMediaSource.Factory(dataSourceFactory)
                    .setDrmSessionManager(drmSessionManager)
                    .createMediaSource(MediaItem.fromUri(uri));
        }
    }

    private void releasePlayer() {
        Log.d(TAG, "Releasing player");

        if (player != null) {
            // Release player resources properly
            player.removeListener(playerListener);
            player.release();
            player = null;
        }
    }

    @Override
    protected void onStart() {
        super.onStart();
        Log.d(TAG, "onStart");
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(TAG, "onPause");
        if (player != null) {
            player.pause(); // Pause player when activity is paused
        }
    }

    @Override
    protected void onStop() {
        super.onStop();
        Log.d(TAG, "onStop");
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy");
        releasePlayer(); // Ensure cleanup when activity is destroyed
    }
}
