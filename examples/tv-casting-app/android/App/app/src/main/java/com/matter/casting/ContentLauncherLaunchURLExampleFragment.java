/*
 *   Copyright (c) 2024 Project CHIP Authors
 *   All rights reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */
package com.matter.casting;

import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.TextView;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import chip.devicecontroller.ChipClusters;
import com.R;
import com.matter.casting.core.CastingPlayer;
import com.matter.casting.core.Endpoint;
import java.util.Optional;

/** A {@link Fragment} to send Content Launcher LaunchURL command using the TV Casting App. */
public class ContentLauncherLaunchURLExampleFragment extends Fragment {
    private static final String TAG = ContentLauncherLaunchURLExampleFragment.class.getSimpleName();
    private static final Integer SAMPLE_ENDPOINT_VID = 65521;

    private final CastingPlayer selectedCastingPlayer;
    private final boolean useCommissionerGeneratedPasscode;

    // Define your video content entries
    private static final VideoContent SAMPLE_VIDEO_1 = new VideoContent(
            "https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4",
            "Big Buck Bunny - By Blender Foundation"
    );

    private static final VideoContent SAMPLE_VIDEO_2 = new VideoContent(
            "https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/ElephantsDream.mp4",
            "Elephants Dream - By Blender Foundation"
    );

    private static final VideoContent SAMPLE_VIDEO_3 = new VideoContent(
            "https://commondatastorage.googleapis.com/gtv-videos-bucket/sample/Sintel.mp4",
            "Sintel - By Blender Foundation"
    );

    private static final VideoContent SAMPLE_IMAGE_GALLERY = new VideoContent(
            "https://picsum.photos/v2/list?page=1&limit=20",
            "Picsum Image Gallery"
    );

    // Static class to hold video content information
    private static class VideoContent {
        final String url;
        final String displayName;

        VideoContent(String url, String displayName) {
            this.url = url;
            this.displayName = displayName;
        }
    }

    public ContentLauncherLaunchURLExampleFragment(
      CastingPlayer selectedCastingPlayer, boolean useCommissionerGeneratedPasscode) {
        Log.d(TAG, "Constructing ContentLauncherLaunchURLExampleFragment");
        this.selectedCastingPlayer = selectedCastingPlayer;
        this.useCommissionerGeneratedPasscode = useCommissionerGeneratedPasscode;
    }

    /**
    * Use this factory method to create a new instance of this fragment using the provided
    * parameters.
    *
    * @param selectedCastingPlayer CastingPlayer that the casting app connected to.
    * @param useCommissionerGeneratedPasscode Boolean indicating whether this CastingPlayer was
    *     commissioned using the Commissioner-Generated Passcode (CGP) commissioning flow.
    * @return A new instance of fragment ContentLauncherLaunchURLExampleFragment.
    */
    public static ContentLauncherLaunchURLExampleFragment newInstance(
      CastingPlayer selectedCastingPlayer, Boolean useCommissionerGeneratedPasscode) {
    return new ContentLauncherLaunchURLExampleFragment(
        selectedCastingPlayer, useCommissionerGeneratedPasscode);
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        Log.d(TAG, "ContentLauncherLaunchURLExampleFragment.onCreateView called");
        // Inflate the layout for this fragment
        return inflater.inflate(R.layout.fragment_matter_content_launcher_launch_url, container, false);
    }

    @Override
    public void onViewCreated(View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        Log.d(TAG, "ContentLauncherLaunchURLExampleFragment.onViewCreated called");

        // Original launch URL button
        View launchUrlButton = view.findViewById(R.id.launchUrlButton);
        if (launchUrlButton != null) {
            launchUrlButton.setOnClickListener(v -> launchVideoContent(getUserInputContent(view)));
        } else {
            Log.e(TAG, "launchUrlButton not found in layout");
        }

        // Sample video 1 button
        View video1Button = view.findViewById(R.id.launchVideo1Button);
        if (video1Button != null) {
            video1Button.setOnClickListener(v -> launchVideoContent(SAMPLE_VIDEO_1));
        } else {
            Log.e(TAG, "launchVideo1Button not found in layout");
        }

        // Sample video 2 button
        View video2Button = view.findViewById(R.id.launchVideo2Button);
        if (video2Button != null) {
            video2Button.setOnClickListener(v -> launchVideoContent(SAMPLE_VIDEO_2));
        } else {
            Log.e(TAG, "launchVideo2Button not found in layout");
        }

        // Sample video 3 button
        View video3Button = view.findViewById(R.id.launchVideo3Button);
        if (video3Button != null) {
            video3Button.setOnClickListener(v -> launchVideoContent(SAMPLE_VIDEO_3));
        } else {
            Log.e(TAG, "launchVideo3Button not found in layout");
        }

        // Sample image gallery button
        View imageGalleryButton = view.findViewById(R.id.launchImageGalleryButton);
        if (imageGalleryButton != null) {
            imageGalleryButton.setOnClickListener(v -> launchVideoContent(SAMPLE_IMAGE_GALLERY));
        } else {
            Log.e(TAG, "launchImageGalleryButton not found in layout");
        }
    }

    private VideoContent getUserInputContent(View view) {
        EditText contentUrlEditText = view.findViewById(R.id.contentUrlEditText);
        String contentUrl = contentUrlEditText != null ? contentUrlEditText.getText().toString() : "";

        EditText contentDisplayStringEditText = view.findViewById(R.id.contentDisplayStringEditText);
        String contentDisplayString = contentDisplayStringEditText != null ?
                contentDisplayStringEditText.getText().toString() : "";

        return new VideoContent(contentUrl, contentDisplayString);
    }

    private void launchVideoContent(VideoContent content) {
        Endpoint endpoint =
            EndpointSelectorExample.selectFirstEndpointByVID(selectedCastingPlayer);

        if (endpoint == null) {
            Log.e(TAG, "No Endpoint with sample vendorID found on CastingPlayer");
            return;
        }

        ChipClusters.ContentLauncherCluster cluster =
                endpoint.getCluster(ChipClusters.ContentLauncherCluster.class);
        if (cluster == null) {
            Log.e(TAG, "Could not get ContentLauncherCluster for endpoint with ID: "
                    + endpoint.getId());
            return;
        }

        cluster.launchURL(
                new ChipClusters.ContentLauncherCluster.LauncherResponseCallback() {
                    @Override
                    public void onSuccess(Integer status, Optional<String> data) {
                        Log.d(TAG, "LaunchURL success. Status: " + status + ", Data: " + data);
                        new Handler(Looper.getMainLooper())
                                .post(() -> {
                                    TextView launcherResult = getView().findViewById(R.id.launcherResult);
                                    launcherResult.setText(
                                            "LaunchURL result\nStatus: " + status + ", Data: " + data);
                                });
                    }

                    @Override
                    public void onError(Exception error) {
                        Log.e(TAG, "LaunchURL failure " + error);
                        new Handler(Looper.getMainLooper())
                                .post(() -> {
                                    TextView launcherResult = getView().findViewById(R.id.launcherResult);
                                    launcherResult.setText("LaunchURL result\nError: " + error);
                                });
                    }
                },
                content.url,
                Optional.of(content.displayName),
                Optional.empty());
    }
}
