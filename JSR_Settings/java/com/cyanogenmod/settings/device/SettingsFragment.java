package com.cyanogenmod.settings.device;

import android.content.ComponentName;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.SystemProperties;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceFragment;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.SwitchPreference;
import android.provider.Settings;
import android.util.Log;
import android.widget.Toast;

import java.io.FileOutputStream;
import java.nio.charset.Charset;

/**
 * Created by prodoomman on 19.02.15.
 */
public class SettingsFragment extends PreferenceFragment implements Preference.OnPreferenceChangeListener, OnPreferenceClickListener {

    private static final String TAG = "JSR_Settings";
    private static final String BTN_FUNC_APP = "btn_func_app";
    private static final String BTN_FUNC_APP2 = "btn_func_app2";
    private static final String PERSISTENT_PROPERTY_CONFIGURATION_NAME = "persist.storages.configuration";
    private static final String USBMSC_PRESENT_PROPERTY_NAME = "ro.usbmsc.present";
    private static final String STORAGES_CONFIGURATION_CLASSIC = "0" ;
    private static final String STORAGES_CONFIGURATION_INVERTED = "1" ;
    private static final String STORAGES_CONFIGURATION_DATAMEDIA = "2" ;
    private static final String BTN_G_CAL = "btn_g_cal";
    private static final String SWT_SPKR = "swt_spkr";
    private static final String SWT_CALL = "swt_call";
    private static final String SWT_VOICE_REC = "swt_voice_rec";
    private static final String SWT_AUDIO_REC = "swt_audio_rec";

    EditTextPreferenceEx btn_func_app;
    EditTextPreferenceEx btn_func_app2;
    Preference btn_g_cal;
    SwitchPreference swt_spkr;
    SwitchPreference swt_call;
    SwitchPreference swt_voice_rec;
    SwitchPreference swt_audio_rec;

    private class SysfsValue {
        private String fileName;
        private String value;

        private SysfsValue(String fileName, String value) {
            this.fileName = fileName;
            this.value = value;
        }

        public String getFileName() {
            return fileName;
        }

        public String getValue() {
            return value;
        }
    }

    class SysfsWriteTask extends AsyncTask<SysfsValue, Void, Integer> {

        @Override
        protected Integer doInBackground(SysfsValue... params) {
            try {
                FileOutputStream fos = new FileOutputStream(params[0].getFileName());
                fos.write(params[0].getValue().getBytes(Charset.forName("UTF-8")));
                fos.close();
            } catch (Exception e) {
                e.printStackTrace();
                return -1;
            }
            return 0;
        }

        @Override
        protected void onPostExecute(Integer result) {
            if(0 != result) {
                Toast.makeText(getActivity(), R.string.fail, Toast.LENGTH_SHORT).show();
            }
        }
    }

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_jsr);
        ListPreference main_storage;
        String configuration;
        boolean usbmscPresent = SystemProperties.getBoolean(USBMSC_PRESENT_PROPERTY_NAME, true);
        if (usbmscPresent) {
            main_storage=(ListPreference)findPreference("main_storage");
            main_storage.setEntries(new String[]{getString(R.string.storage_datamedia), getString(R.string.storage_usbmsc), getString(R.string.storage_sdcard)});
            main_storage.setEntryValues(new String[]{STORAGES_CONFIGURATION_DATAMEDIA, STORAGES_CONFIGURATION_CLASSIC, STORAGES_CONFIGURATION_INVERTED});
            configuration = SystemProperties.get(PERSISTENT_PROPERTY_CONFIGURATION_NAME, STORAGES_CONFIGURATION_CLASSIC);
        } else {
            main_storage=(ListPreference)findPreference("main_storage");
            main_storage.setEntries(new String[]{getString(R.string.storage_datamedia), getString(R.string.storage_sdcard)});
            main_storage.setEntryValues(new String[]{STORAGES_CONFIGURATION_DATAMEDIA, STORAGES_CONFIGURATION_INVERTED});
            configuration = SystemProperties.get(PERSISTENT_PROPERTY_CONFIGURATION_NAME, STORAGES_CONFIGURATION_DATAMEDIA);
            if (configuration == STORAGES_CONFIGURATION_CLASSIC) {
                configuration = STORAGES_CONFIGURATION_DATAMEDIA;
            }
        }

        main_storage.setOnPreferenceChangeListener(this);

        main_storage.setValue(configuration);

        btn_func_app = (EditTextPreferenceEx)findPreference(BTN_FUNC_APP);
        btn_func_app.setOnPreferenceChangeListener(this);

        btn_func_app2 = (EditTextPreferenceEx)findPreference(BTN_FUNC_APP2);
        btn_func_app2.setOnPreferenceChangeListener(this);

        btn_g_cal = (Preference)findPreference(BTN_G_CAL);
        btn_g_cal.setOnPreferenceClickListener(this);

        swt_spkr = (SwitchPreference)findPreference(SWT_SPKR);
        swt_call = (SwitchPreference)findPreference(SWT_CALL);
        swt_voice_rec = (SwitchPreference)findPreference(SWT_VOICE_REC);
        swt_audio_rec = (SwitchPreference)findPreference(SWT_AUDIO_REC);
        swt_spkr.setOnPreferenceChangeListener(this);
        swt_call.setOnPreferenceChangeListener(this);
        swt_voice_rec.setOnPreferenceChangeListener(this);
        swt_audio_rec.setOnPreferenceChangeListener(this);
        swt_spkr.setChecked(SystemProperties.getBoolean("persist.audio.fluence.speaker", false));
        swt_call.setChecked(SystemProperties.getBoolean("persist.audio.fluence.voicecall", false));
        swt_voice_rec.setChecked(SystemProperties.getBoolean("persist.audio.fluence.voicerec", false));
        swt_audio_rec.setChecked(SystemProperties.getBoolean("persist.audio.fluence.audiorec", false));
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue)
    {
        if (preference.getKey().equals("main_storage")) {
            SystemProperties.set(PERSISTENT_PROPERTY_CONFIGURATION_NAME, (String)newValue);
            Toast.makeText(getActivity(), R.string.reboot_needed, Toast.LENGTH_LONG).show();
        }
        if (preference.getKey().equals(BTN_FUNC_APP)) {
            Settings.System.putString(getActivity().getContentResolver(), preference.getKey(), (String)newValue);
        }
        if (preference.getKey().equals(BTN_FUNC_APP2)) {
            Settings.System.putString(getActivity().getContentResolver(), preference.getKey(), (String)newValue);
        }
        if (preference.getKey().equals(SWT_SPKR)) {
            SystemProperties.set("persist.audio.fluence.speaker", newValue.toString());
            Toast.makeText(getActivity(), R.string.reboot_needed, Toast.LENGTH_LONG).show();
        }
        if (preference.getKey().equals(SWT_CALL)) {
            SystemProperties.set("persist.audio.fluence.voicecall", newValue.toString());
            Toast.makeText(getActivity(), R.string.reboot_needed, Toast.LENGTH_LONG).show();
        }
        if (preference.getKey().equals(SWT_VOICE_REC)) {
            SystemProperties.set("persist.audio.fluence.voicerec", newValue.toString());
            Toast.makeText(getActivity(), R.string.reboot_needed, Toast.LENGTH_LONG).show();
        }
        if (preference.getKey().equals(SWT_AUDIO_REC)) {
            SystemProperties.set("persist.audio.fluence.audiorec", newValue.toString());
            Toast.makeText(getActivity(), R.string.reboot_needed, Toast.LENGTH_LONG).show();
        }
        return true;
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        if (preference.getKey().equals(BTN_G_CAL)) {
            try {
                Intent callGCal = new Intent();
                callGCal.setComponent(new ComponentName("com.qualcomm.sensors.qsensortest",
                                                        "com.qualcomm.sensors.qsensortest.TabControl"));
                startActivity(callGCal);
            } catch (Exception ex) {
                Log.e(TAG, "Failed to start GravityCalibrationActivity: " + ex);
                Toast.makeText(getActivity(), R.string.btn_g_cal_failure, Toast.LENGTH_LONG).show();
            }
        }
        return true;
    }
}
