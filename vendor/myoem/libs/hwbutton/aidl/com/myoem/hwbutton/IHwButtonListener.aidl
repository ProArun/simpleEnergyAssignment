package com.myoem.hwbutton;

// Cross-process callback from HwButtonManagerService (system_server) back to
// HwButtonManager (app process). Kept separate from the native-layer
// IHwButtonServiceCallback (services/hwbutton) since this one crosses the
// system_server <-> app boundary, not the native-service <-> HAL boundary.
oneway interface IHwButtonListener {
    void onClicked();
}
