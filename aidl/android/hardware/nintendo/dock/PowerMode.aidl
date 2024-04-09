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

/**
 * PowerMode will represent the indices read from per-sku config files
 * to avoid sku-specific hacks in the service.
 */

//@VintfStability
@Backing(type="byte")
enum PowerMode {
    MAX_PERF = 0,
    PERF = 1,
    NV_STOCK = 2,
    HOS_STOCK = 3,
    ECO = 4,
}
