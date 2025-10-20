package com.retr0engine.retr0;

import android.util.Log;

public class Retr0Activity extends org.libsdl.app.SDLActivity {
    @Override
    public void loadLibraries() {
        Log.i("Retr0", "Loading native lib: main");
		System.loadLibrary("vulkan"); 
        System.loadLibrary("main"); // SDL3 is statically linked into libmain.so
    }
}
