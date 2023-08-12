/**
 * File              : YDConnect.cpp
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 14.09.2022
 * Last Modified Date: 12.08.2023
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#include "cYandexOAuth/cYandexOAuth.h"

#include <jni.h>
#include <cstdlib>
#include <android/log.h>

#define ERROR(...) ({char ___msg[BUFSIZ]; sprintf(___msg, __VA_ARGS__); __android_log_write(ANDROID_LOG_ERROR, "YDConnect", ___msg);});

#ifdef __cplusplus
extern "C" {
#endif

struct JNI_callback_data {
	JNIEnv *env;
	jobject obj;
	void * ptr;
};

JNIEXPORT jstring JNICALL
Java_kuzm_ig_astroybat_YDConnect_authUrl(JNIEnv* env, jobject obj, jstring client_id) {
	char * error = NULL;
	char * url = c_yandex_oauth_code_url(env->GetStringUTFChars(client_id, 0));
	jstring str =  env->NewStringUTF(url);
	free(url);
	return str;
}

JNIEXPORT jstring JNICALL
Java_kuzm_ig_astroybat_YDConnect_getCode(JNIEnv* env, jobject obj, jstring html) {
	char * error = NULL;
	char * code = c_yandex_oauth_code_from_html(env->GetStringUTFChars(html, 0));
	jstring str =  env->NewStringUTF(code);
	free(code);
	return str;
}

JNIEXPORT void JNICALL
Java_kuzm_ig_astroybat_YDConnect_getToken(JNIEnv* env, jobject obj, jstring code, jstring client_id, jstring client_secret, jstring device_name) {
	struct JNI_callback_data data = {
		.env = env, .obj = obj
	};
	c_yandex_oauth_get_token(
			env->GetStringUTFChars(code, 0),
			env->GetStringUTFChars(client_id, 0),
			env->GetStringUTFChars(client_secret, 0),
			env->GetStringUTFChars(device_name, 0),
			&data,
			[](auto _data, auto access_token, auto expires_in, auto refresh_token, auto error){
				struct JNI_callback_data *data = static_cast<JNI_callback_data *>(_data);
				JNIEnv *env = data->env; jobject obj = data->obj;

				if (error){
					ERROR("%s\n", error);
				} else {
					/* set token to database struct */
					stroybat_set_yandex_token(access_token);

					/* run callback in Java code */
					jmethodID callback = env->GetMethodID(env->GetObjectClass(obj), "getTokenCallback", "(Ljava/lang/String;JLjava/lang/String;)V");
					env->CallVoidMethod (obj, callback,
								env->NewStringUTF(access_token),	
								(long)expires_in,
								env->NewStringUTF(refresh_token)	
							);
				}				

				return 0;
			}
	);
}

#ifdef __cplusplus
}  /* end of the 'extern "C"' block */
#endif
