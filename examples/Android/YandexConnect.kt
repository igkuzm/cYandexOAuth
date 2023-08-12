/**
 * File              : YandexConnect.kt
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 14.09.2022
 * Last Modified Date: 12.08.2023
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

package kuzm.ig.PACKAGE

import android.content.Context
import android.content.Intent
import android.graphics.Bitmap
import android.os.Bundle
import android.util.Log
import android.view.ViewGroup
import android.webkit.JavascriptInterface
import android.webkit.WebView
import android.webkit.WebViewClient
import android.webkit.WebResourceRequest
import android.widget.Toast
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.material.*
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowBack
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.viewinterop.AndroidView
import androidx.compose.ui.Modifier
import androidx.compose.ui.Alignment
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.*

class YConnect : ComponentActivity() {

	companion object {
		init {
			System.loadLibrary("cYandexOAuth")
		}
	}	
	external fun authUrl(client_id: String): String?	
	external fun getCode(html: String): String?	
	external fun getToken(code: String, client_id: String, client_secret: String, device_name: String)	

	override fun onCreate(savedInstanceState: Bundle?) {
		super.onCreate(savedInstanceState)

		//get extra
		val client_id = intent.getStringExtra("client_id")
		val client_secret = intent.getStringExtra("client_secret")

		val deviceName = android.os.Build.MANUFACTURER + " " + android.os.Build.MODEL
		
		setContent {
			ContentView(
				{Text("Yandex Disk")}, 
				navigationIcon = {
					IconButton(
						onClick = {onBackPressed()}
					){
						Icon(Icons.Filled.ArrowBack, "")
					}
				}, 
				actions = {}
			) {
				if (client_id != null && client_secret != null){
					val auth_url = authUrl(client_id)
					if (auth_url != null) {
						OpenWebView(
							auth_url,
							onPageFinished = { view, _ ->
								view.evaluateJavascript("document.body.innerHTML") { html ->
									Log.i("WEB", html)
									val code = getCode(html)
									if (code != null){
										//ok - we have a code
										Log.i("YDCommect", "VERIFICATION CODE: " + code)
										getToken(code, client_id, client_secret, deviceName)
									}
								}							
							}
						)
					}
				} else {
					Text("No client_id or client_secret")
				}
			}
		}
	}

	fun getTokenCallback(access_token: String, expires_in: Long, refresh_token: String) {
		Log.i("YDConnect", "ACCESS TOKEN: " + access_token)
		val intent = Intent()
		intent.putExtra("access_token", access_token)
		intent.putExtra("expires_in", expires_in)
		intent.putExtra("refresh_token", refresh_token)
		setResult(RESULT_OK, intent)
		finish()
	}
	
}

@Composable
fun OpenWebView(
	url: String,
	onPageStarted: ((view: WebView, url: String, favicon: Bitmap?) -> Unit)? = null,
	onPageLoading: ((view: WebView, request: WebResourceRequest) -> Unit)? = null, 
	onPageFinished: ((view: WebView, url: String) -> Unit)? = null
){
	var loading = remember { mutableStateOf(true) }
	Box {
		AndroidView(
			factory = { context ->
				WebView(context).apply {
					layoutParams = ViewGroup.LayoutParams(
						ViewGroup.LayoutParams.MATCH_PARENT,
						ViewGroup.LayoutParams.MATCH_PARENT
					)
					
					settings.javaScriptEnabled = true
					
					webViewClient = object: WebViewClient() {
						override fun onPageStarted(view: WebView, url: String, favicon: Bitmap?) {
							loading.value = true
							onPageStarted?.invoke(view, url, favicon)
						}
						override fun shouldOverrideUrlLoading(view: WebView,  request: WebResourceRequest): Boolean {
							onPageLoading?.invoke(view, request)
							loading.value = true
							return false
						}
						override fun onPageFinished(view: WebView, url: String) {
							loading.value = false
							onPageFinished?.invoke(view, url)
						}
					}

					loadUrl(url)
				}
			},update = {
				it.loadUrl(url)
			}
		)
		Box(
			contentAlignment = Alignment.Center,
			modifier = Modifier
				.matchParentSize()
				//.background(Color.White)
		) {
			if (loading.value){
				CircularProgressIndicator()
			}
		}		
	}
}
