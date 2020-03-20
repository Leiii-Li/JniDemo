package com.nelson.jnicallback;

import android.util.Log;
import android.view.View;
import androidx.annotation.Keep;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    int hour = 0;
    int minute = 0;
    int second = 0;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    private TextView mTickView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method

        mTickView = findViewById(R.id.tickView);


    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String startTickets();

    public native void stopTickets();

    @Keep
    private void updateTimer() {
        ++second;
        if (second >= 60) {
            ++minute;
            second -= 60;
            if (minute >= 60) {
                ++hour;
                minute -= 60;
            }
        }
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                String ticks = "" + MainActivity.this.hour + ":" +
                    MainActivity.this.minute + ":" +
                    MainActivity.this.second;
                MainActivity.this.mTickView.setText(ticks);
            }
        });
    }

    public void onClick(View view) {
        switch (view.getId()) {
            case R.id.start:
                startTickets();
                break;
            case R.id.stop:
                stopTickets();
                break;
        }
    }
}
