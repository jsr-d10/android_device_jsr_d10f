package com.cyanogenmod.settings.device;

import android.database.ContentObserver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.PowerManager;
import android.os.SystemClock;
import android.provider.Settings.System;
import android.view.KeyEvent;
import android.util.Slog;

import com.android.internal.os.DeviceKeyHandler;

public class KeyHandler implements DeviceKeyHandler
{
  private static final String TAG = KeyHandler.class.getSimpleName();

  private final Context mContext;
  private final PowerManager mPowerManager;
  private final Handler mHandler;

  private CameraActivationAction cameraAction = null;
  //private TorchAction torchAction = null;
    
  private SettingsObserver mSettingsObserver;
  
  private static final String BTN_FUNC_APP = "btn_func_app";
  private String btn_func_app;
  private static final String BTN_FUNC_APP2 = "btn_func_app2";
  private String btn_func_app2;

  private static long btn_func_timestamp = 0;
  private static final int BTN_FUNC_TIMESTAMP_DELTA = 400;

  class SettingsObserver extends ContentObserver
  {
    SettingsObserver(Handler handler)
    {
      super(handler);
    }

    void observe()
    {
      KeyHandler.this.mContext.getContentResolver().registerContentObserver(System.getUriFor(BTN_FUNC_APP), false, this);
      KeyHandler.this.mContext.getContentResolver().registerContentObserver(System.getUriFor(BTN_FUNC_APP2), false, this);
    }

    public void onChange(boolean selfChange)
    {
      KeyHandler.this.updateSettings();
    }
  }
  
  public void updateSettings()
  {
    btn_func_app = System.getString(mContext.getContentResolver(), BTN_FUNC_APP);
    if (btn_func_app != null)
      btn_func_app = btn_func_app.trim();
    btn_func_app2 = System.getString(mContext.getContentResolver(), BTN_FUNC_APP2);
    if (btn_func_app2 != null)
      btn_func_app2 = btn_func_app2.trim();
  }   
   
  public KeyHandler(Context context)
  {
    mContext = context;
    mPowerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
    mHandler = new Handler();
    mSettingsObserver = new SettingsObserver(mHandler);
    mSettingsObserver.observe();
    updateSettings();
    Slog.i(TAG, "Init com.cyanogenmod.keyhandler");
  }

  public boolean handleKeyEvent(KeyEvent event)
  {
    boolean consumed = false;
    switch (event.getKeyCode()) {
    case KeyEvent.KEYCODE_FUNCTION:
      if (event.getAction() == KeyEvent.ACTION_DOWN) {
        //Slog.i(TAG, "FUNC key down " + event.getRepeatCount());
        if (btn_func_timestamp == 0){
          btn_func_timestamp = java.lang.System.currentTimeMillis();
          Slog.i(TAG, "FUNC key down at "+ btn_func_timestamp);
        }
        //consumed = true;
      }
      if (event.getAction() == KeyEvent.ACTION_UP && btn_func_timestamp > 0) {
        String uri;
        long new_timestamp = java.lang.System.currentTimeMillis();
        if (new_timestamp - btn_func_timestamp < BTN_FUNC_TIMESTAMP_DELTA) {
          Slog.i(TAG, "FUNC key up fast at "+ new_timestamp +", delta: "+ (new_timestamp - btn_func_timestamp));
          uri = btn_func_app;
        } else {
          Slog.i(TAG, "FUNC key up long at "+ new_timestamp +", delta: "+ (new_timestamp - btn_func_timestamp));
          uri = btn_func_app2;
        }
        btn_func_timestamp = 0;
        if (uri != null && uri.length() > 0) {
          if (cameraAction == null)
            cameraAction = new CameraActivationAction(mContext, 50);
          Slog.i(TAG, "launch app '" + uri + "'");
          if (uri.equals("camera")) {
            cameraAction.action();
          } else {
            cameraAction.actionEx(uri);
          }
          consumed = true;
        }        
      }
    }
    return consumed;
  }
}
