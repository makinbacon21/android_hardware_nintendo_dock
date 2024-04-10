# [WIP] DockService

DockService is an AIDL-based Android service for handling power profile and (eventually) display switching for CPU, GPU, and optionally EMC clocks on Nintendo Switch devices (should be usable for all Tegra210 devices). End goal is drop-in replacement for the important parts of NvCPL.

## Purpose

NvCPL is a proprietary, bloated, and closed-source set of services that is less functional with every Android release it is ported to, but serves the important purpose of handling power profiles (similar to `nvpmodel` on L4T). It also mucks about with inputs and HDMI connections, which makes it frustrating to work around for docking devices like `nx`. This service is being developed to replace NvCPL's helpful functionality with an OSS minimalistic interface.

## Anatomy

- Polling thread using `epoll` to query charging (`power_supply`) state
- Profile switcher based on AIDL interface
- (Next) Sysfs knob controls based on profiles
