package com.myoem.hwbutton;

import com.myoem.hwbutton.IHwButtonServiceCallback;

interface IHwButtonService {
    int readValue();
    void triggerClick();
    void registerCallback(in IHwButtonServiceCallback callback);
}
