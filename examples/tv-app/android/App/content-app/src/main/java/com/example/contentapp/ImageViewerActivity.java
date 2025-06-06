package com.example.contentapp;

import android.content.Intent;
import android.os.Bundle;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import androidx.viewpager2.widget.ViewPager2;
import com.android.volley.Request;
import com.android.volley.toolbox.JsonArrayRequest;
import com.android.volley.toolbox.Volley;
import com.matter.tv.app.api.MatterIntentConstants;

import org.json.JSONException;
import org.json.JSONObject;
import java.util.ArrayList;
import java.util.List;

public class ImageViewerActivity extends AppCompatActivity {
    private ViewPager2 viewPager;
    private ImagePagerAdapter adapter;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_image_viewer);

        viewPager = findViewById(R.id.viewPager);
        findViewById(R.id.closeButton).setOnClickListener(v -> finish());

        Intent intent = getIntent();
        String galleryUrl = intent.getStringExtra("gallery_url");
        fetchPicsumImages(galleryUrl);
    }

    private void fetchPicsumImages(String galleryUrl) {
        JsonArrayRequest request = new JsonArrayRequest(Request.Method.GET, galleryUrl, null,
                response -> {
                    try {
                        List<String> imageUrls = new ArrayList<>();

                        for (int i = 0; i < response.length(); i++) {
                            JSONObject photo = response.getJSONObject(i);
                            String imageUrl = photo.getString("download_url");
                            imageUrls.add(imageUrl);
                        }

                        adapter = new ImagePagerAdapter(imageUrls.toArray(new String[0]));
                        viewPager.setAdapter(adapter);

                    } catch (JSONException e) {
                        e.printStackTrace();
                        Toast.makeText(this, "Error parsing gallery",
                                Toast.LENGTH_SHORT).show();
                    }
                },
                error -> {
                    error.printStackTrace();
                    Toast.makeText(this, "Error loading gallery",
                            Toast.LENGTH_SHORT).show();
                });

        Volley.newRequestQueue(this).add(request);
    }
}
