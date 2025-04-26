package com.example.contentapp;

import android.os.Bundle;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.google.android.exoplayer2.SimpleExoPlayer;
import com.google.android.exoplayer2.MediaItem;
import com.google.android.exoplayer2.ExoPlaybackException;
import com.google.android.exoplayer2.Player;
import com.google.android.exoplayer2.ui.PlayerView;

public class VideoPlayerActivity extends AppCompatActivity {
    private PlayerView playerView;
    private SimpleExoPlayer player;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_video_player);

        // Close button
        findViewById(R.id.closeButton).setOnClickListener(v -> {
            finish();  // This will close the activity and return to previous screen
        });

        // Player view
        playerView = findViewById(R.id.player_view);
        String videoUrl = getIntent().getStringExtra("video_url");
        initializePlayer(videoUrl);
    }

    private void initializePlayer(String videoUrl) {
        // Create player instance
        player = new SimpleExoPlayer.Builder(this).build();

        // Bind the player to the view
        playerView.setPlayer(player);

        // Create media item
        MediaItem mediaItem = MediaItem.fromUri(videoUrl);

        // Set the media item to be played
        player.setMediaItem(mediaItem);

        // Prepare the player
        player.prepare();

        // Start the video automatically (optional)
        player.play();

        addPlayerListeners();
    }

    private void addPlayerListeners() {
        player.addListener(new Player.Listener() {
            public void onPlayerError(ExoPlaybackException error) {
                String errorMessage = "An error occurred: " + error.getMessage();
                Toast.makeText(VideoPlayerActivity.this, errorMessage, Toast.LENGTH_LONG).show();
            }
        });
    }

    @Override
    protected void onStart() {
        super.onStart();
        if (player != null) {
            player.play();
        }
    }

    @Override
    protected void onStop() {
        super.onStop();
        if (player != null) {
            player.pause();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (player != null) {
            player.release();
            player = null;
        }
    }
}
