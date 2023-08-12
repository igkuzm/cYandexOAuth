/**
 * File              : cYandexOAuth.c
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 12.08.2023
 * Last Modified Date: 12.08.2023
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#define VERIFY_SSL 0

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "uuid4.h"
#include "cJSON.h"
#include "cYandexOAuth.h"

char * c_yandex_oauth_code_url(const char *client_id) {
	
	if (!client_id){
		perror("client_id is NULL");
		return NULL;
	}

	char *s = (char *)malloc(BUFSIZ);
	if (!s){
		perror("malloc");
		return NULL;
	}
	
	sprintf(s, 
			"https://oauth.yandex.ru/authorize?response_type=code"	
			"&client_id=%s", client_id);
	
	return s;
}

// find position of needle string in haystack
static long _strfnd( 
		const char * haystack, 
		const char * needle
		)
{
	const char *p = strstr(haystack, needle);
	if (p)
		return p - haystack;
	return -1;
}

static char * port_listner(
		int port,
		void * user_data,
		void (*callback)(
			void * user_data,
			const char * access_token,
			int expires_in,
			const char * refresh_token,
			const char * error
			)
		)
{
    int socket_desc, client_sock, client_size;
    struct sockaddr_in server_addr, client_addr;
    char server_message[2000], client_message[2000];

    // Clean buffers:
    memset(server_message, '\0', sizeof(server_message));
    memset(client_message, '\0', sizeof(client_message));

    // Create socket:
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);

    if(socket_desc < 0){
				callback(user_data, NULL, 0, NULL,
						"Error while creating socket");
        return NULL;
    }
    //printf("Socket created successfully\n");

    // Set port and IP:
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Bind to the set port and IP:
    if(bind(socket_desc, (struct sockaddr*)&server_addr, sizeof(server_addr))<0){
				callback(user_data, NULL, 0, NULL,
						"Couldn't bind to the port");
        return NULL;
    }
    //printf("Done with binding\n");

    // Listen for clients:
    if(listen(socket_desc, 1) < 0){
				callback(user_data, NULL, 0, NULL,
						"Error while listening");
        return NULL;
    }
    //printf("\nListening for incoming connections.....\n");

    // Accept an incoming connection:
    client_size = sizeof(client_addr);
    client_sock = accept(socket_desc, 
				(struct sockaddr*)&client_addr, &client_size);

    if (client_sock < 0){
				callback(user_data, NULL, 0, NULL,
						"Can't accept socet connection");
				close(socket_desc);
        return NULL;
    }
    //printf("Client connected at IP: %s and port: %i\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    // Receive client's message:
    if (recv(client_sock, client_message, sizeof(client_message), 0) < 0){
				callback(user_data, NULL, 0, NULL,
						"Couldn't receive incoming message");
				close(client_sock);
				close(socket_desc);
        return NULL;
    }
		client_message[sizeof(client_message)-1] = 0;
    //printf("Msg from client: %s\n", client_message);
		
		//find start of verification code
		const char * pattern = "code="; 
		int len = strlen(pattern);
		int start = _strfnd(client_message, pattern); 
		if (start < 0){
			callback(user_data, NULL, 0, NULL,
					"Couldn't find verification code in message");
			return NULL;
		}
		//find end of code
		long end = _strfnd(&client_message[start], " ");

		//find length of verification code
		long clen = end - len;

		//allocate code and copy
		char * code = malloc(clen + 1);
		if (!code){
			callback(user_data, NULL, 0, NULL,
					"memory allocation error");
			return NULL;
		}
		strncpy(code, &client_message[start + len], clen);
		code[clen] = 0;

		// Respond to client:
    strcpy(server_message, "Done!");
		send(client_sock, server_message, strlen(server_message), 0);

    // Closing the socket:
    close(client_sock);
    close(socket_desc);

    return code;
}

struct string {
	char *ptr;
	size_t len;
};

static int init_string(struct string *s) 
{
	s->len = 0;
	s->ptr = (char *)malloc(s->len+1);
	if (!s->ptr){
		perror("malloc");
		return -1;
	}
	s->ptr[0] = '\0';
	return 0;
}

static size_t writefunc(
		void *ptr, size_t size, size_t nmemb, struct string *s)
{
	size_t new_len = s->len + size*nmemb;
	s->ptr = (char *)realloc(s->ptr, new_len+1);
	if (!s->ptr){
		perror("realloc");
		return 0;
	}
	memcpy(s->ptr+s->len, ptr, size*nmemb);
	s->ptr[new_len] = '\0';
	s->len = new_len;

	return size*nmemb;
}

void c_yandex_oauth_get_token(
		const char *verification_code, 
		const char *client_id, 
		const char *client_secret, 
		const char *device_name, 
		void * user_data,
		void (*callback)(
			void * user_data,
			const char * access_token,
			int expires_in,
			const char * refresh_token,
			const char * error
			)
		)
{
	if (!callback){
		perror("callback is NULL");
		return;
	}

	if (verification_code == NULL) {
		callback(user_data, NULL, 0, NULL, 
				"cYandexDisk: No verification_code");
		return;
	}

	char device_id[37];
	UUID4_STATE_T state; UUID4_T uuid;
	uuid4_seed(&state);
	uuid4_gen(&state, &uuid);
	if (!uuid4_to_s(uuid, device_id, 37)){
		callback(user_data, NULL, 0, NULL, 
				"cYandexDisk: Can't genarate UUID");
		return;
	}
	
	CURL *curl = curl_easy_init();
		
	struct string s;
	init_string(&s);

	if(curl) {
		char requestString[] = "https://oauth.yandex.ru/token";	
		
		curl_easy_setopt(curl, CURLOPT_URL, requestString);
		curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");		
		curl_easy_setopt(curl, CURLOPT_HEADER, 0);

		struct curl_slist *header = NULL;
	  header = curl_slist_append(header, "Connection: close");		
	  header = curl_slist_append(header, 
				"Content-Type: application/x-www-form-urlencoded");		
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
		
		char post[BUFSIZ];
		sprintf(post, "grant_type=authorization_code");		
		sprintf(post, "%s&code=%s",				post, verification_code);
		sprintf(post, "%s&client_id=%s",		post, client_id);
		sprintf(post, "%s&client_secret=%s",	post, client_secret);
		sprintf(post, "%s&device_id=%s",		post, device_id);
		sprintf(post, "%s&device_name=%s",		post, device_name);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post));
	    
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, VERIFY_SSL);		

		CURLcode res = curl_easy_perform(curl);

		const char *error = curl_easy_strerror(res);
		curl_easy_cleanup(curl);
		curl_slist_free_all(header);
	
		if (res) { //handle erros
			callback(user_data, NULL, 0, NULL, error);
			free(s.ptr);
			return;			
		}			

		//parse JSON answer
		cJSON *json = cJSON_ParseWithLength(s.ptr, s.len);
		free(s.ptr);
		if (cJSON_IsObject(json)) {
			cJSON *access_token = 
					cJSON_GetObjectItem(json, "access_token");			
			if (!access_token) { //handle errors
				cJSON *error_description = 
						cJSON_GetObjectItem(json, "error_description");
				if (!error_description) {
					callback(user_data, NULL, 0, NULL, 
							"cYandexDisk: unknown error!");
					cJSON_free(json);
					return;
				}
				callback(user_data, NULL, 0, NULL, 
						error_description->valuestring);
				cJSON_free(json);
				return;
			}
			//OK - we have a token
			callback(
					user_data, 
					access_token->valuestring, 
					cJSON_GetObjectItem(json, "expires_in")->valueint, 
					cJSON_GetObjectItem(json, "refresh_token")->valuestring, 
					NULL);
			cJSON_free(json);
		}	
	}
}

void c_yandex_oauth_listen_port_for_code(
		const char *client_id, 
		const char *client_secret, 
		const char *device_name, 
		int port,
		void * user_data,
		void (*callback)(
			void * user_data,
			const char * access_token,
			int expires_in,
			const char * refresh_token,
			const char * error
			)
		)
{
	if (!callback){
		perror("callback is NULL");
		return;
	}

	if (client_id == NULL) {
		callback(user_data, NULL, 0, NULL, "No client_id");
		return;
	}
	
	if (client_secret == NULL) {
		callback(user_data, NULL, 0, NULL, "No client_secret");
		return;
	}
	
	if (device_name == NULL) {
		callback(user_data, NULL, 0, NULL, "No deivce_name");
		return;
	}

	char *code = port_listner(port, user_data, callback);
	if (code)
		c_yandex_oauth_get_token(
				code, client_id, client_secret, 
				device_name, user_data, callback);
}


char *c_yandex_oauth_code_from_html(
		const char *html)
{
	if (!html){
		perror("html is NULL");
		return NULL;
	}

	const char * patterns[] = {
		"verification_code%3Fcode%3D",
		"class=\"verification-code-code\">"
	};
	const char *pattern_ends[] = {
		"&",
		"<"
	};	

	int i;
	for (int i = 0; i < 2; i++) {
		const char * s = patterns[i]; 
		int len = strlen(s);

		//find start of verification code class structure in html
		long start = _strfnd(html, s); 
		if (start >= 0){
			//find end of code
			long end = _strfnd(&html[start], pattern_ends[i]);

			//find length of verification code
			long clen = end - len;

			//allocate code and copy
			char * code = (char *)malloc(clen + 1);
			if (!code){
				perror("malloc");
				return NULL;
			}

			strncpy(code, &html[start + len], clen);
			code[clen] = 0;

			return code;
		}
	}
	return NULL;
}

void c_yandex_oauth_on_page(
		const char *client_id, 
		const char *device_name,
		void * user_data,
		void (*callback)(
			void * user_data,
			const char * device_code,
			const char * user_code,
			const char * verification_url,
			int interval,
			int expires_in,
			const char * error
			)
		)
{
	if (!callback){
		perror("callback is NULL");
		return;
	}

	if (client_id == NULL) {
		callback(user_data, NULL, NULL, NULL, 0, 0, "No client_id");
		return;
	}
	
	if (device_name == NULL) {
		callback(user_data, NULL, NULL, NULL, 0, 0, "No deivce_name");
		return;
	}

	char device_id[37];
	UUID4_STATE_T state; UUID4_T uuid;
	uuid4_seed(&state);
	uuid4_gen(&state, &uuid);
	if (!uuid4_to_s(uuid, device_id, 37)){
		callback(user_data, NULL, NULL, NULL, 0, 0, 
				"Can't genarate UUID");
		return;
	}
	
	CURL *curl = curl_easy_init();
	if (!curl){
		callback(user_data, NULL, NULL, NULL, 0, 0, "cURL init error");
		return;
	}
		
	struct string s;
	if (init_string(&s))
		return;

	char requestString[] = "https://oauth.yandex.ru/device/code";	
	
	curl_easy_setopt(curl, CURLOPT_URL, requestString);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");		
	curl_easy_setopt(curl, CURLOPT_HEADER, 0);

	struct curl_slist *header = NULL;
	header = curl_slist_append(header, "Connection: close");		
	header = curl_slist_append(header, 
			"Content-Type: application/x-www-form-urlencoded");		
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
	
	char post[BUFSIZ];
	sprintf(post, "%s&client_id=%s",		post, client_id);
	sprintf(post, "%s&device_id=%s",		post, device_id);
	sprintf(post, "%s&device_name=%s",	post, device_name);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post));
		
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, VERIFY_SSL);		

	CURLcode res = curl_easy_perform(curl);
	
	const char *error = curl_easy_strerror(res);
	curl_easy_cleanup(curl);
	curl_slist_free_all(header);
	
	if (res) { //handle erros
		callback(user_data, NULL, NULL, NULL, 0, 0, error);
		free(s.ptr);
		return;			
	}		
	//parse JSON answer
	cJSON *json = cJSON_ParseWithLength(s.ptr, s.len);
	free(s.ptr);
	if (cJSON_IsObject(json)) {
		cJSON *device_code = cJSON_GetObjectItem(json, "device_code");			
		if (!device_code) { //handle errors
			cJSON *error_description = cJSON_GetObjectItem(json, 
					"error_description");
			if (!error_description) {
				callback(user_data, NULL, NULL, NULL, 0, 0, 
						"unknown error!"); 
				cJSON_free(json);
				return;
			}
			callback(user_data, NULL, NULL, NULL, 0, 0, 
					error_description->valuestring); 
			cJSON_free(json);
			return;
		}
		//OK - we have a code
		callback(user_data, 
				device_code->valuestring, 
				cJSON_GetObjectItem(json, "user_code")->valuestring, 
				cJSON_GetObjectItem(json, "verification_url")->valuestring, 
				cJSON_GetObjectItem(json, "interval")->valueint, 
				cJSON_GetObjectItem(json, "expires_in")->valueint, 
				NULL
				);
		cJSON_free(json);
	}	
}

void c_yandex_oauth_ask_token_in_interval(
		const char *device_code, 
		const char *client_id,    
		const char *client_secret,
		int interval,
		int expires_in,
		void * user_data,
		int (*callback)(
			void * user_data,
			const char * access_token,
			int expires_in,
			const char * refresh_token,
			const char * error
			)
	)
{	
	if (!callback){
		perror("callback is NULL");
		return;
	}

	if (device_code == NULL) {
		callback(user_data, NULL, 0, NULL, "No device_code");
		return;
	}
	if (client_id == NULL) {
		callback(user_data, NULL, 0, NULL, "No client_id");
		return;
	}
	if (client_secret == NULL) {
		callback(user_data, NULL, 0, NULL, "No client_secret");
		return;
	}

	char device_id[37];
	UUID4_STATE_T state; UUID4_T uuid;
	uuid4_seed(&state);
	uuid4_gen(&state, &uuid);
	if (!uuid4_to_s(uuid, device_id, 37)){
		callback(user_data, NULL, 0, NULL, "Can't genarate UUID");
		return;
	}
	
	CURL *curl = curl_easy_init();
	if (!curl){
		callback(user_data, NULL, 0, NULL, "cURL init error");
		return;
	}
		
	struct string s;
	if(init_string(&s))
		return;

	char requestString[] = "https://oauth.yandex.ru/token";	
	
	curl_easy_setopt(curl, CURLOPT_URL, requestString);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");		
	curl_easy_setopt(curl, CURLOPT_HEADER, 0);

	struct curl_slist *header = NULL;
	header = curl_slist_append(header, "Connection: close");		
	header = curl_slist_append(header, 
			"Content-Type: application/x-www-form-urlencoded");		
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
	
	char post[BUFSIZ];
	sprintf(post, "grant_type=device_code");		
	sprintf(post, "%s&code=%s",				  post, device_code);
	sprintf(post, "%s&client_id=%s",		post, client_id);
	sprintf(post, "%s&client_secret=%s",post, client_secret);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(post));
		
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, VERIFY_SSL);		

	int i;
	for (i=0; i < expires_in; i += interval){
		
		CURLcode res = curl_easy_perform(curl);

		const char *error = curl_easy_strerror(res);
		curl_easy_cleanup(curl);
		curl_slist_free_all(header);
	
		if (res) { //handle erros
			callback(user_data, NULL, 0, NULL, error);
			free(s.ptr);
			continue;			
		}			
		//parse JSON answer
		cJSON *json = cJSON_ParseWithLength(s.ptr, s.len);
		free(s.ptr);
		if (cJSON_IsObject(json)) {
			cJSON *access_token = 
					cJSON_GetObjectItem(json, "access_token");			
			if (!access_token) { //handle errors
				cJSON *error_description = 
						cJSON_GetObjectItem(json, "error_description");
				if (!error_description) {
					if (callback(user_data, NULL, 0, NULL, 
								"unknown error!")){
						cJSON_free(json);
						return;
					}
					cJSON_free(json);
					continue;
				}
				if (callback(user_data, NULL, 0, NULL, 
							error_description->valuestring)){
					cJSON_free(json);
					return;
				}
				cJSON_free(json);
				continue;
			}
			//OK - we have a token
			callback(
					user_data, 
					access_token->valuestring, 
					cJSON_GetObjectItem(json, "expires_in")->valueint, 
					cJSON_GetObjectItem(json, "refresh_token")->valuestring, 
					NULL);
			cJSON_free(json);
			break;
		}	
		sleep(interval);
	}
}
// vim:ft=c
