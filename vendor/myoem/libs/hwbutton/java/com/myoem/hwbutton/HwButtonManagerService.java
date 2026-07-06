package com.myoem.hwbutton;

import android.content.Context;
import android.os.IBinder;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Log;

import com.android.server.SystemService;

// bridges the native hwbuttond daemon to a framework-facing binder; apps look
// up HW_BUTTON_SERVICE_NAME via ServiceManager instead of getSystemService()
public class HwButtonManagerService extends SystemService {
    private static final String TAG = "HwButtonManagerService";
    private static final String NATIVE_SERVICE_NAME =
            "com.myoem.hwbutton.IHwButtonService/default";

    // duplicated instead of referencing HwButtonManager.HW_BUTTON_SERVICE
    // (separate Soong module, would be a backwards dependency) - must match
    private static final String HW_BUTTON_SERVICE_NAME = "hwbutton";

    private final RemoteCallbackList<IHwButtonListener> mListeners = new RemoteCallbackList<>();
    private volatile IHwButtonService mNative;

    private final IBinder.DeathRecipient mDeathRecipient = this::reconnect;

    private final IHwButtonServiceCallback mNativeCallback = new IHwButtonServiceCallback.Stub() {
        @Override
        public void onClicked() {
            int n = mListeners.beginBroadcast();
            for (int i = 0; i < n; i++) {
                try {
                    mListeners.getBroadcastItem(i).onClicked();
                } catch (RemoteException e) {
                    Log.w(TAG, "listener callback failed", e);
                }
            }
            mListeners.finishBroadcast();
        }
    };

    public HwButtonManagerService(Context context) {
        super(context);
    }

    @Override
    public void onStart() {
        reconnect();
        publishBinderService(HW_BUTTON_SERVICE_NAME, new BinderService());
        Log.i(TAG, "published " + HW_BUTTON_SERVICE_NAME);
    }

    private void reconnect() {
        IBinder binder = ServiceManager.waitForService(NATIVE_SERVICE_NAME);
        if (binder == null) {
            Log.e(TAG, "hwbuttond not available at " + NATIVE_SERVICE_NAME);
            return;
        }
        try {
            binder.linkToDeath(mDeathRecipient, 0);
        } catch (RemoteException e) {
            Log.e(TAG, "hwbuttond died before linkToDeath completed", e);
            reconnect();
            return;
        }
        IHwButtonService svc = IHwButtonService.Stub.asInterface(binder);
        try {
            svc.registerCallback(mNativeCallback);
        } catch (RemoteException e) {
            Log.e(TAG, "failed to register native callback", e);
        }
        mNative = svc;
        Log.i(TAG, "connected to " + NATIVE_SERVICE_NAME);
    }

    private class BinderService extends IHwButtonManager.Stub {
        @Override
        public int readValue() throws RemoteException {
            IHwButtonService svc = mNative;
            if (svc == null) {
                throw new RemoteException("hwbuttond not connected");
            }
            return svc.readValue();
        }

        @Override
        public void trigger() throws RemoteException {
            IHwButtonService svc = mNative;
            if (svc == null) {
                throw new RemoteException("hwbuttond not connected");
            }
            svc.triggerClick();
        }

        @Override
        public void registerListener(IHwButtonListener listener) {
            mListeners.register(listener);
        }

        @Override
        public void unregisterListener(IHwButtonListener listener) {
            mListeners.unregister(listener);
        }
    }
}
