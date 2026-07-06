package com.myoem.hwbuttondemo

import android.os.Bundle
import android.os.ServiceManager
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.lifecycle.ViewModelProvider
import com.myoem.hwbutton.HwButtonManager
import com.myoem.hwbuttondemo.ui.HwButtonScreen
import com.myoem.hwbuttondemo.ui.theme.HwButtonDemoTheme

// ServiceManager.getService() is @hide, kept isolated to this one call site
class MainActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val binder = ServiceManager.getService(HwButtonManager.HW_BUTTON_SERVICE)
        val manager = HwButtonManager(binder)

        val viewModel = ViewModelProvider(
            this,
            HwButtonViewModelFactory(manager)
        )[HwButtonViewModel::class.java]

        setContent {
            HwButtonDemoTheme {
                HwButtonScreen(viewModel = viewModel)
            }
        }
    }
}
