package com.myoem.hwbuttondemo

import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import com.myoem.hwbutton.HwButtonManager
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

data class UiState(
    val value: Int? = null,
    val serviceAvailable: Boolean = false,
    val justClicked: Boolean = false,
)

sealed class UiEvent {
    data object Read : UiEvent()
    data object Trigger : UiEvent()
}

// open so tests can subclass without mockito-inline
open class HwButtonViewModel(private val manager: HwButtonManager) : ViewModel() {

    private val _uiState = MutableStateFlow(UiState(serviceAvailable = manager.isAvailable()))
    open val uiState: StateFlow<UiState> = _uiState.asStateFlow()

    init {
        manager.registerListener(
            object : HwButtonManager.HwButtonListener {
                override fun onClicked() {
                    _uiState.update { it.copy(justClicked = true) }
                }
            }
        )
    }

    open fun onEvent(event: UiEvent) {
        when (event) {
            is UiEvent.Read -> read()
            is UiEvent.Trigger -> trigger()
        }
    }

    private fun read() {
        viewModelScope.launch(Dispatchers.IO) {
            val v = manager.readValue()
            _uiState.update { it.copy(value = v, serviceAvailable = manager.isAvailable()) }
        }
    }

    private fun trigger() {
        viewModelScope.launch(Dispatchers.IO) {
            manager.trigger()
            // kernel resets to 0 on trigger, re-read to avoid a stale value
            val v = manager.readValue()
            _uiState.update { it.copy(value = v, serviceAvailable = manager.isAvailable()) }
        }
    }

    fun consumeClickedFlag() {
        _uiState.update { it.copy(justClicked = false) }
    }
}

class HwButtonViewModelFactory(
    private val manager: HwButtonManager
) : ViewModelProvider.Factory {
    @Suppress("UNCHECKED_CAST")
    override fun <T : ViewModel> create(modelClass: Class<T>): T =
        HwButtonViewModel(manager) as T
}
