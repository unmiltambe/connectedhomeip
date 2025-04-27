package com.example.contentapp;

import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import com.google.android.exoplayer2.SimpleExoPlayer;
import com.google.android.exoplayer2.MediaItem;
import com.google.android.exoplayer2.Player;
import com.google.android.exoplayer2.ui.PlayerView;

public class VideoPlayerActivity extends AppCompatActivity {
    private static final String TAG = "VideoPlayerActivity";
    private PlayerView playerView;
    private SimpleExoPlayer player;
    private final Player.Listener playerListener = new Player.Listener() {
        @Override
        public void onPlayerError(com.google.android.exoplayer2.PlaybackException error) {
            String errorMessage = "An error occurred: " + error.getLocalizedMessage();
            Toast.makeText(VideoPlayerActivity.this, errorMessage, Toast.LENGTH_LONG).show();
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate");
        setContentView(R.layout.activity_video_player);

        playerView = findViewById(R.id.player_view);

        String videoUrl = getIntent().getStringExtra("video_url");
        Log.d(TAG, "Video URL: " + videoUrl);

        findViewById(R.id.closeButton).setOnClickListener(v -> {
            Log.d(TAG, "Close button clicked");
            finish();
        });

        initializePlayer(videoUrl);
    }

    private void initializePlayer(String videoUrl) {
        Log.d(TAG, "Initializing player");

        player = new SimpleExoPlayer.Builder(this).build();
        playerView.setPlayer(player); // Link the PlayerView with ExoPlayer

        // Set the media item
        MediaItem mediaItem = MediaItem.fromUri(videoUrl);
        player.setMediaItem(mediaItem);
        player.prepare();
        player.play();

        player.addListener(playerListener);
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
