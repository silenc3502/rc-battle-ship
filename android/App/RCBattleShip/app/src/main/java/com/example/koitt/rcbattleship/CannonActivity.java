package com.example.koitt.rcbattleship;

import android.content.Intent;
import android.graphics.Bitmap;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore;
import android.view.View;
import android.widget.ImageView;
import android.widget.Toast;

/**
 * Created by koitt on 17. 7. 12.
 */

public class CannonActivity extends MainActivity{
    private static int PICK_IMAGE_REQUEST = 1; ImageView imgView; static final String TAG = "MainActivity";


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_cannon);
    }
    public void loadImagefromGallery(View view) {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.setType("image/*");
        startActivityForResult(Intent.createChooser(intent, "Select Picture"), PICK_IMAGE_REQUEST);
    }
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        try {
            if (requestCode == PICK_IMAGE_REQUEST && resultCode == RESULT_OK && null != data) {
                Uri uri = data.getData();
                Bitmap bitmap = MediaStore.Images.Media.getBitmap(getContentResolver(),uri);
                int nh = (int) (bitmap.getHeight() * (1024.0 / bitmap.getWidth()));
                Bitmap scaled = Bitmap.createScaledBitmap(bitmap, 1024, nh, true);
                imgView = (ImageView) findViewById(R.id.imageView);
                imgView.setImageBitmap(scaled);
            } else {
                Toast.makeText(this, "취소 되었습니다.", Toast.LENGTH_LONG).show();
            }
        } catch (Exception e) {
            Toast.makeText(this, "Oops! 로딩에 오류가 있습니다.", Toast.LENGTH_LONG).show(); e.printStackTrace();
        }
    }
}



