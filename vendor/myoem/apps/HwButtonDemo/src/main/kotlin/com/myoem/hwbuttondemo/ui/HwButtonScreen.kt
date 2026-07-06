package com.myoem.hwbuttondemo.ui

import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.myoem.hwbuttondemo.HwButtonViewModel
import com.myoem.hwbuttondemo.R
import com.myoem.hwbuttondemo.UiEvent
import com.myoem.hwbuttondemo.UiState

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun HwButtonScreen(viewModel: HwButtonViewModel) {
    val uiState by viewModel.uiState.collectAsState()

    LaunchedEffect(uiState.justClicked) {
        if (uiState.justClicked) {
            kotlinx.coroutines.delay(1_500)
            viewModel.consumeClickedFlag()
        }
    }

    Scaffold(
        topBar = { TopAppBar(title = { Text(stringResource(R.string.app_name)) }) }
    ) { padding ->
        Column(
            modifier = Modifier
                .padding(padding)
                .fillMaxSize()
                .padding(horizontal = 16.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.Center
        ) {
            if (!uiState.serviceAvailable && uiState.value == null) {
                ServiceUnavailableCard()
                Spacer(Modifier.height(24.dp))
            }

            ValueCard(uiState)

            Spacer(Modifier.height(32.dp))

            Row(horizontalArrangement = Arrangement.spacedBy(16.dp)) {
                Button(onClick = { viewModel.onEvent(UiEvent.Read) }) {
                    Text(stringResource(R.string.read))
                }
                Button(onClick = { viewModel.onEvent(UiEvent.Trigger) }) {
                    Text(stringResource(R.string.trigger))
                }
            }

            Spacer(Modifier.height(16.dp))

            if (uiState.justClicked) {
                Text(
                    text = stringResource(R.string.clicked_toast),
                    color = MaterialTheme.colorScheme.primary,
                    style = MaterialTheme.typography.titleMedium,
                    fontWeight = FontWeight.Bold
                )
            }
        }
    }
}

@Composable
private fun ServiceUnavailableCard() {
    Card(
        colors = CardDefaults.cardColors(containerColor = MaterialTheme.colorScheme.errorContainer)
    ) {
        Text(
            text = stringResource(R.string.service_unavailable),
            modifier = Modifier.padding(16.dp),
            color = MaterialTheme.colorScheme.error
        )
    }
}

@Composable
private fun ValueCard(state: UiState) {
    Card {
        Column(
            modifier = Modifier.padding(24.dp),
            horizontalAlignment = Alignment.CenterHorizontally
        ) {
            Text(
                text = stringResource(R.string.value_label),
                style = MaterialTheme.typography.titleMedium
            )
            Spacer(Modifier.height(8.dp))
            Text(
                text = state.value?.toString() ?: "-",
                fontSize = 48.sp,
                fontWeight = FontWeight.Bold
            )
        }
    }
}
