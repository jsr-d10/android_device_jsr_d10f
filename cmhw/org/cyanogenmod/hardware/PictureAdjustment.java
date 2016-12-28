/*
 * Copyright (C) 2016 S-trace <S-trace@list.ru> and The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.cyanogenmod.hardware;

import android.annotation.TargetApi;
import android.util.Log;
import android.util.Range;

import cyanogenmod.hardware.HSIC;
import org.cyanogenmod.internal.util.FileUtils;

/**
 * Picture adjustment support
 *
 * Allows tuning of hue, saturation, intensity, and contrast levels
 * of the display
 */
public class PictureAdjustment {
    private static final String TAG = "S-trace";

    /**
     * http://forum.xda-developers.com/android/software-hacking/dev-kcal-advanced-color-control-t3032080
     * /sys/devices/platform/kcal_ctrl.0/kcal - (0-256 0-256 0-256) - Controls R/G/B Multipliers
     * /sys/devices/platform/kcal_ctrl.0/kcal_min - (0-256) - Controls minimum RGB Multiplier value
     * /sys/devices/platform/kcal_ctrl.0/kcal_enable - (0-1) - Enables/Disables RGB Multiplier Control
     * /sys/devices/platform/kcal_ctrl.0/kcal_invert - (0-1) - Enables/Disables Display Inversion Mode
     * /sys/devices/platform/kcal_ctrl.0/kcal_sat - (224-383 or 128) - Controls saturation intensity - use      * 128 for grayscale mode
     * /sys/devices/platform/kcal_ctrl.0/kcal_hue - (0-1536) - Controls display hue - may have issues with      * msm8x26 in the higher values
     * /sys/devices/platform/kcal_ctrl.0/kcal_val - (128-383) - Controls display value
     * /sys/devices/platform/kcal_ctrl.0/kcal_cont - (128-383) - Controls display contrast
     */
    private static final String KCAL_ENABLE_FILE = "/sys/devices/platform/kcal_ctrl.0/kcal_enable";
    private static final String KCAL_HUE_FILE = "/sys/devices/platform/kcal_ctrl.0/kcal_hue";
    private static final String KCAL_SAT_FILE = "/sys/devices/platform/kcal_ctrl.0/kcal_sat";
    private static final String KCAL_VAL_FILE = "/sys/devices/platform/kcal_ctrl.0/kcal_val";
    private static final String KCAL_CNT_FILE = "/sys/devices/platform/kcal_ctrl.0/kcal_cont";
    private static final HSIC mHsic = new HSIC(0.0f, 255.0f, 255.0f, 255.0f, 255.0f);
    private static final HSIC mHsicDefault = new HSIC(0.0f, 255.0f, 255.0f, 255.0f, 255.0f);

     
    /**
     * Whether device supports picture adjustment
     *
     * @return boolean Supported devices must return always true
     */
    public static boolean isSupported() {
        boolean supported = FileUtils.isFileWritable(KCAL_ENABLE_FILE) &&
                FileUtils.isFileWritable(KCAL_HUE_FILE) &&
                FileUtils.isFileWritable(KCAL_SAT_FILE) &&
                FileUtils.isFileWritable(KCAL_VAL_FILE) &&
                FileUtils.isFileWritable(KCAL_CNT_FILE);
        Log.d(TAG, "isSupported()=" + supported);
        return supported;

    }

    /**
     * This method returns the current picture adjustment values based
     * on the selected DisplayMode.
     *
     * @return the HSIC object or null if not supported
     */
    public static HSIC getHSIC() {
        Log.d(TAG, "getHSIC");
        return mHsic;
    }

    /**
     * This method returns the default picture adjustment values.
     *
     * If DisplayModes are available, this may change depending on the
     * selected mode.
     *
     * @return the HSIC object or null if not supported
     */
    public static HSIC getDefaultHSIC() {
        Log.d(TAG, "getDefaultHSIC");
        return mHsicDefault;
    }


    /**
     * This method allows to set the picture adjustment
     *
     * @param hsic
     * @return boolean Must be false if feature is not supported or the operation
     * failed; true in any other case.
     */
    public static boolean setHSIC(final HSIC hsic) {
        Log.d(TAG, "setHSIC: " + hsic.toString());
        return FileUtils.writeLine(KCAL_HUE_FILE, String.valueOf((int)hsic.getHue())) &&
               FileUtils.writeLine(KCAL_SAT_FILE, String.valueOf((int)hsic.getSaturation())) &&
               FileUtils.writeLine(KCAL_SAT_FILE, String.valueOf((int)hsic.getIntensity())) &&
               FileUtils.writeLine(KCAL_CNT_FILE, String.valueOf((int)hsic.getContrast()));
    }

    /**
     * Get the range available for hue adjustment
     * @return range of floats
     */
    public static Range<Float> getHueRange() {
        Log.d(TAG, "getHueRange");
        return new Range(0.0f, 1536.0f);
    }

    /**
     * Get the range available for saturation adjustment
     * @return range of floats
     */
    public static Range<Float> getSaturationRange() {
        Log.d(TAG, "getSaturationRange");
        return new Range(224.0f, 383.0f);
    }

    /**
     * Get the range available for intensity adjustment
     * @return range of floats
     */
    public static Range<Float> getIntensityRange() {
        Log.d(TAG, "getIntensityRange");
        return new Range(128.0f, 383.0f);
    }

    /**
     * Get the range available for contrast adjustment
     * @return range of floats
     */
    public static Range<Float> getContrastRange() {
        Log.d(TAG, "getContrastRange");
        return new Range(128.0f, 383.0f);
    }

    /**
     * Get the range available for saturation threshold adjustment
     *
     * This is the threshold where the display becomes fully saturated
     *
     * @return range of floats
     */
    public static Range<Float> getSaturationThresholdRange() {
        Log.d(TAG, "getSaturationThresholdRange");
        return new Range(224.0f, 383.0f);
    }
}
