package com.myoem.hwbutton;

import com.myoem.hwbutton.IHwButtonListener;

// Binder interface published by HwButtonManagerService under
// HwButtonManager.HW_BUTTON_SERVICE. HwButtonManager (app-facing API) wraps
// this interface — apps never see it directly.
interface IHwButtonManager {
    int readValue();
    void trigger();
    void registerListener(in IHwButtonListener listener);
    void unregisterListener(in IHwButtonListener listener);
}
