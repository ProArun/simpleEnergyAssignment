package vendor.myoem.hardware.hwbutton;

import vendor.myoem.hardware.hwbutton.IHwButtonHalCallback;

interface IHwButtonHal {
    int getValue();
    void trigger();
    void registerCallback(in IHwButtonHalCallback callback);
}
