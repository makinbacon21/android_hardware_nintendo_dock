/*
 * Copyright (C) 2024 Thomas Makin
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

package android.hardware.nintendo.dock;

import android.hardware.nintendo.dock.PowerMode;

/**
 * Dock interface definition.
 */
//@VintfStability
interface IDock {
    /**
     * Sets the target PowerMode profile. This sets max CPU/GPU/EMC freqs for the
     * current governors. PowerMode is essentially an index to read from a device-
     * specific configuration (provided by implementation). This should also
     * clear forced freq if currently forced.
     *
     * @param mode PowerMode profile index to set
     */
    void setPowerMode(in PowerMode mode);

    /**
     * Forces CPU/GPU to prefer max freq given by the provided mode.
     *
     * @param mode PowerMode profile index to set
     */
    void forceModeFreq(in PowerMode mode);

    /**
     * Retrieves the currently active PowerMode in use by the system.
     *
     * @return PowerMode currently active profile
     */
    PowerMode getPowerMode();

    /**
     * Gets available PowerModes. This is dependent on what was provided in device-
     * specific config and any other state that may affect what can be set.
     *
     * @return PowerMode[] array of currently available modes
     */
    PowerMode[] getAvailableModes();

    /**
     * Gets available CPU freqs. This is dependent on what was provided in device-
     * specific config and any other state that may affect what can be set
     * 
     * Implementation may desire to return the union of the device-specific modes
     * with the read available values from sysfs.
     *
     * @return int[] array of available CPU freq values
     */
    int[] getAvailableCpuFreqs();
    
    /**
     * Gets available GPU freqs. This is dependent on what was provided in device-
     * specific config and any other state that may affect what can be set
     * 
     * Implementation may desire to return the union of the device-specific modes
     * with the read available values from sysfs.
     *
     * @return int[] array of available GPU freq values
     */
    int[] getAvailableGpuFreqs();

    /**
     * Gets "docked" state. This should require both external display and USB
     * host connection.
     *
     * @return boolean whether the system is "docked"
     */
    boolean getDockedState();
}
