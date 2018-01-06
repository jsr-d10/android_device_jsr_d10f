package com.cyanogenmod.settings.device;

import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;
import android.os.SystemProperties;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceFragment;
import android.preference.PreferenceGroup;
import android.preference.SwitchPreference;
import android.provider.Settings;
import android.util.Log;
import android.widget.Toast;

import java.util.ArrayList;

/**
 * Created by prodoomman on 19.02.15.
 * Implements functionality for EditTextPreference
 */
public class SettingsFragment extends PreferenceFragment
        implements Preference.OnPreferenceChangeListener, OnPreferenceClickListener {

    private static final String TAG = "JSR_Settings";
    private static final String FUNC_CATEGORY_KEY = "btn_func_cat";
    private static final String BTN_FUNC_APP_KEY = "btn_func_app";
    private static final String BTN_FUNC_APP2_KEY = "btn_func_app2";

    private static final String STORAGES_CONFIGURATION_CLASSIC = "0";
    private static final String STORAGES_CONFIGURATION_INVERTED = "1";
    private static final String STORAGES_CONFIGURATION_DATAMEDIA = "2";

    private static final String MAIN_STORAGE_CATEGORY = "main_storage_category";
    private static final String PREFERENCE_MAIN_STORAGE_KEY = "main_storage";
    private static final String PREFERENCE_G_SENSOR_CALIBRATION_KEY = "button_g_sensor_calibration";
    private static final String SWITCH_ANC_SPEAKER_KEY = "switch_anc_speaker";
    private static final String SWITCH_ANC_CALL_KEY = "switch_anc_call";
    private static final String SWITCH_ANC_VOICE_RECORD_KEY = "switch_anc_voice_record";
    private static final String SWITCH_ANC_AUDIO_RECORD_KEY = "switch_anc_audio_record";

    private static final String PROPERTY_CONFIGURATION_NAME = "persist.storages.configuration";
    private static final String PROPERTY_EMMC_HAS_USBMSC_NAME = "ro.emmc.has.usbmsc";
    private static final String PROPERTY_SD_HAS_USBMSC_NAME = "ro.sd.has.usbmsc";
    private static final String PROPERTY_SD_HAS_PLAIN_PART_NAME = "ro.sd.has.plain_part";
    private static final String PROPERTY_ANC_SPEAKER_NAME = "persist.audio.fluence.speaker";
    private static final String PROPERTY_ANC_CALL_NAME = "persist.audio.fluence.voicecall";
    private static final String PROPERTY_ANC_VOICE_REC_NAME = "persist.audio.fluence.voicerec";
    private static final String PROPERTY_ANC_AUDIO_REC_NAME = "persist.audio.fluence.audiorec";

    private String currentRom;
    private boolean emmcHasUsbmsc;
    private boolean sdHasUsbmsc;
    private boolean sdHasPlainPart;
    private boolean ancSpeakerEnabled;
    private boolean ancCallEnabled;
    private boolean ancVoiceRecordEnabled;
    private boolean ancAudioRecordEnabled;
    private String[] storageConfigurationEntries;
    private String[] storageConfigurationEntryValues;

    private void readProperties() {
        emmcHasUsbmsc = SystemProperties.getBoolean(PROPERTY_EMMC_HAS_USBMSC_NAME, false);
        sdHasUsbmsc = SystemProperties.getBoolean(PROPERTY_SD_HAS_USBMSC_NAME, false);
        sdHasPlainPart = SystemProperties.getBoolean(PROPERTY_SD_HAS_PLAIN_PART_NAME, false);
        currentRom = SystemProperties.get("ro.ota.current_rom", "");
        ancSpeakerEnabled = SystemProperties.getBoolean(PROPERTY_ANC_SPEAKER_NAME, false);
        ancCallEnabled = SystemProperties.getBoolean(PROPERTY_ANC_CALL_NAME, false);
        ancVoiceRecordEnabled = SystemProperties.getBoolean(PROPERTY_ANC_VOICE_REC_NAME, false);
        ancAudioRecordEnabled = SystemProperties.getBoolean(PROPERTY_ANC_AUDIO_REC_NAME, false);
    }

    private String prepareStoragesConfiguration() {
        ArrayList<String> entries = new ArrayList<>();
        ArrayList<String> values = new ArrayList<>();

        String configuration = SystemProperties.get(
                PROPERTY_CONFIGURATION_NAME,
                STORAGES_CONFIGURATION_CLASSIC
        );

        // Datamedia is the only always possible configuration
        entries.add(getString(R.string.storage_datamedia));
        values.add(STORAGES_CONFIGURATION_DATAMEDIA);

        if (emmcHasUsbmsc) {
            entries.add(getString(R.string.storage_usbmsc));
            values.add(STORAGES_CONFIGURATION_CLASSIC);
        } else {
            if (STORAGES_CONFIGURATION_CLASSIC.equals(configuration)) {
                if (sdHasPlainPart || sdHasUsbmsc)
                    configuration = STORAGES_CONFIGURATION_INVERTED;
                else
                    configuration = STORAGES_CONFIGURATION_DATAMEDIA;
            }
        }

        if (sdHasUsbmsc || sdHasPlainPart) {
            entries.add(getString(R.string.storage_sdcard));
            values.add(STORAGES_CONFIGURATION_INVERTED);
        } else {
            if (STORAGES_CONFIGURATION_INVERTED.equals(configuration)) {
                if (emmcHasUsbmsc)
                    configuration = STORAGES_CONFIGURATION_CLASSIC;
                else
                    configuration = STORAGES_CONFIGURATION_DATAMEDIA;
            }
        }

        storageConfigurationEntries = new String[entries.size()];
        entries.toArray(storageConfigurationEntries);
        storageConfigurationEntryValues = new String[values.size()];
        values.toArray(storageConfigurationEntryValues);

        return configuration;
    }

    private void showToast(int text) {
        Toast.makeText(getActivity(), text, Toast.LENGTH_LONG).show();
    }

    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.pref_jsr);

        readProperties();

        String configuration = prepareStoragesConfiguration();
        ListPreference main_storage = (ListPreference) findPreference(PREFERENCE_MAIN_STORAGE_KEY);
        main_storage.setOnPreferenceChangeListener(this);
        main_storage.setEntries(storageConfigurationEntries);
        main_storage.setEntryValues(storageConfigurationEntryValues);
        main_storage.setValue(configuration);
        int possibleStoragesConfigurations = storageConfigurationEntries.length;
        Log.d("S-trace", "possibleStoragesConfigurations=" + possibleStoragesConfigurations);
        if (possibleStoragesConfigurations == 1) {
            PreferenceGroup preferenceGroup;
            preferenceGroup = (PreferenceGroup) findPreference(MAIN_STORAGE_CATEGORY);
            if (preferenceGroup != null) {
                preferenceGroup.removePreference(main_storage);
                getPreferenceScreen().removePreference(preferenceGroup);
            }
        }

        EditTextPreferenceEx btn_func_app;
        btn_func_app = (EditTextPreferenceEx) findPreference(BTN_FUNC_APP_KEY);
        btn_func_app.setOnPreferenceChangeListener(this);

        EditTextPreferenceEx btn_func_app2;
        btn_func_app2 = (EditTextPreferenceEx) findPreference(BTN_FUNC_APP2_KEY);
        btn_func_app2.setOnPreferenceChangeListener(this);

        // This will not work on MIUI anyway, so let's just hide it
        if (currentRom.toLowerCase().contains("miui")) {
            PreferenceGroup preferenceGroup = (PreferenceGroup) findPreference(FUNC_CATEGORY_KEY);
            if (preferenceGroup != null) {
                preferenceGroup.removePreference(btn_func_app);
                preferenceGroup.removePreference(btn_func_app2);
                getPreferenceScreen().removePreference(preferenceGroup);
            }
        }

        Preference buttonGSensorCalibration = findPreference(PREFERENCE_G_SENSOR_CALIBRATION_KEY);
        buttonGSensorCalibration.setOnPreferenceClickListener(this);

        SwitchPreference switchAncSpeaker;
        switchAncSpeaker = (SwitchPreference) findPreference(SWITCH_ANC_SPEAKER_KEY);
        switchAncSpeaker.setOnPreferenceChangeListener(this);
        switchAncSpeaker.setChecked(ancSpeakerEnabled);

        SwitchPreference switchAncCall;
        switchAncCall = (SwitchPreference) findPreference(SWITCH_ANC_CALL_KEY);
        switchAncCall.setOnPreferenceChangeListener(this);
        switchAncCall.setChecked(ancCallEnabled);

        SwitchPreference switchAncVoiceRecord;
        switchAncVoiceRecord = (SwitchPreference) findPreference(SWITCH_ANC_VOICE_RECORD_KEY);
        switchAncVoiceRecord.setOnPreferenceChangeListener(this);
        switchAncVoiceRecord.setChecked(ancVoiceRecordEnabled);

        SwitchPreference switchAncAudioRecord;
        switchAncAudioRecord = (SwitchPreference) findPreference(SWITCH_ANC_AUDIO_RECORD_KEY);
        switchAncAudioRecord.setOnPreferenceChangeListener(this);
        switchAncAudioRecord.setChecked(ancAudioRecordEnabled);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        String preferenceKey = preference.getKey();
        switch (preferenceKey) {
            case PREFERENCE_MAIN_STORAGE_KEY:
                SystemProperties.set(PROPERTY_CONFIGURATION_NAME, (String) newValue);
                showToast(R.string.reboot_needed);
                break;
            case BTN_FUNC_APP_KEY:
                Settings.System.putString(getActivity().getContentResolver(),
                        preference.getKey(), (String) newValue);
                break;
            case BTN_FUNC_APP2_KEY:
                Settings.System.putString(getActivity().getContentResolver(),
                        preference.getKey(), (String) newValue);
                break;
            case SWITCH_ANC_SPEAKER_KEY:
                SystemProperties.set(PROPERTY_ANC_SPEAKER_NAME, newValue.toString());
                showToast(R.string.reboot_needed);
                break;
            case SWITCH_ANC_CALL_KEY:
                SystemProperties.set(PROPERTY_ANC_CALL_NAME, newValue.toString());
                showToast(R.string.reboot_needed);
                break;
            case SWITCH_ANC_VOICE_RECORD_KEY:
                SystemProperties.set(PROPERTY_ANC_VOICE_REC_NAME, newValue.toString());
                showToast(R.string.reboot_needed);
                break;
            case SWITCH_ANC_AUDIO_RECORD_KEY:
                SystemProperties.set(PROPERTY_ANC_AUDIO_REC_NAME, newValue.toString());
                showToast(R.string.reboot_needed);
                break;
        }
        return true;
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        if (PREFERENCE_G_SENSOR_CALIBRATION_KEY.equals(preference.getKey())) {
            try {
                Intent intent = new Intent();
                ComponentName component = new ComponentName(
                        "com.qualcomm.sensors.qsensortest",
                        "com.qualcomm.sensors.qsensortest.TabControl"
                );
                intent.setComponent(component);
                startActivity(intent);
            } catch (Exception ex) {
                Log.e(TAG, "Failed to start GravityCalibrationActivity: " + ex);
                showToast(R.string.btn_g_cal_failure);
            }
        }
        return true;
    }
}
