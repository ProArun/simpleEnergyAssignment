package com.myoem.hwbutton;

import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.RemoteException;
import android.util.Log;

// caller looks up the @hide ServiceManager binder and passes it in here
public class HwButtonManager {
    private static final String TAG = "HwButtonManager";

    // must match HwButtonManagerService.HW_BUTTON_SERVICE_NAME
    public static final String HW_BUTTON_SERVICE = "hwbutton";

    public interface HwButtonListener {
        void onClicked();
    }

    private final IHwButtonManager mService;
    private final Handler mMainHandler = new Handler(Looper.getMainLooper());

    private final IHwButtonListener mBinderListener = new IHwButtonListener.Stub() {
        @Override
        public void onClicked() {
            mMainHandler.post(() -> {
                synchronized (mListeners) {
                    for (HwButtonListener l : mListeners) {
                        l.onClicked();
                    }
                }
            });
        }
    };

    private final java.util.List<HwButtonListener> mListeners = new java.util.ArrayList<>();

    public HwButtonManager(IBinder binder) {
        if (binder != null) {
            mService = IHwButtonManager.Stub.asInterface(binder);
        } else {
            mService = null;
            Log.w(TAG, "constructed with a null binder — all calls will be no-ops");
        }
    }

    public boolean isAvailable() {
        return mService != null;
    }

    public int readValue() {
        if (mService == null) return -1;
        try {
            return mService.readValue();
        } catch (RemoteException e) {
            Log.e(TAG, "readValue failed", e);
            return -1;
        }
    }

    public void trigger() {
        if (mService == null) return;
        try {
            mService.trigger();
        } catch (RemoteException e) {
            Log.e(TAG, "trigger failed", e);
        }
    }

    public void registerListener(HwButtonListener listener) {
        if (mService == null || listener == null) return;
        synchronized (mListeners) {
            boolean wasEmpty = mListeners.isEmpty();
            mListeners.add(listener);
            if (wasEmpty) {
                try {
                    mService.registerListener(mBinderListener);
                } catch (RemoteException e) {
                    Log.e(TAG, "registerListener failed", e);
                }
            }
        }
    }

    public void unregisterListener(HwButtonListener listener) {
        if (mService == null || listener == null) return;
        synchronized (mListeners) {
            mListeners.remove(listener);
            if (mListeners.isEmpty()) {
                try {
                    mService.unregisterListener(mBinderListener);
                } catch (RemoteException e) {
                    Log.e(TAG, "unregisterListener failed", e);
                }
            }
        }
    }
}
