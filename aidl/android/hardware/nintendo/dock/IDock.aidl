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
    void setPowerMode(in PowerMode mode);

    PowerMode getPowerMode();

    PowerMode[] getAvailableModes();

    int[] getAvailableCpuFreqs();
    
    int[] getAvailableGpuFreqs();

    boolean getDockedState();
}
