/**
 * File              : cYandexOAuth.h
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 12.08.2023
 * Last Modified Date: 12.08.2023
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

#ifndef C_YANDEX_OAUTH_H
#define C_YANDEX_OAUTH_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Получение OAuth-токенов
 * Яндекс авторизует приложения с помощью OAuth-токенов. 
 * Каждый токен предоставляет определенному приложению доступ 
 * к данным определенного пользователя.
 *
 * Чтобы использовать протокол OAuth:
 * 1. Зарегистрируйте свое OAuth-приложение.
 *		https://oauth.yandex.ru/client/new
 *		Вы получите client_id и client_secret
 * 
 * 2. Подучите код подтверждения и обменяйте его на токен 
 *		выберите один из способов:
 *		
 *		2.1 Получение кода подтверждения из URL перенаправления:
 *				2.1.1 В настройках приложения на https://oauth.yandex.ru/client
 *				указать Redirect URI http://localhost:'номер порта'
 *				2.1.2 Получить url от c_yandex_oauth_code_url
 *				2.1.3 Запустить c_yandex_oauth_listen_port_for_code
 *				2.1.4 Пользователь должен открыть url в браузере, авторизоваться.
 *				Код автоматически будет переправлен на указанный порт и 
 *				обменян на токен.
 *
 *		2.2 Получение кода подтверждения от пользователя:
 *				2.2.1 В настройках приложения на https://oauth.yandex.ru/client
 *				указать Redirect URI https://oauth.yandex.ru/verification_code
 *				2.2.2 Получить url от c_yandex_oauth_code_url
 *				2.2.3 Пользователь должен открыть url в браузере, авторизоваться
 *				и получить код - будет выведен на экран. 
 *				Для получения кода програмным способом, необходимо прочитать HTML
 *				(примеры в examples). Получить код из HTML можно:
 *				c_yandex_oauth_code_from_html
 *				2.2.4 Обменять код на токен c_yandex_oauth_get_token
 *
 *		2.3 Ввод кода на странице авторизации Яндекс OAuth
 *				2.3.1 Получить device_code, user_code и url 
 *				для получения кода подтверждения от
 *				c_yandex_oauth_on_page
 *				2.3.2 Попросить пользователя перейти на страницу
 *				url, авторизоваться и ввести user_code. Одновременно запусить
 *				c_yandex_oauth_ask_token_in_interval
*/

/* return allocated c null-terminated string
 * with url to get oauth code or NULL on error*/
char * c_yandex_oauth_code_url(const char *client_id);

/* listen port for code from yandex server
 * and exchange code on token
 * %device_name - any non-NULL string
 * %port - http://localhost:port - you should config in
 * https://oauth.yandex.ru/client */
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
		);

/* return allocated c null-terminated string
 * with oauth code from HTML or NULL on error*/
char * c_yandex_oauth_code_from_html(const char *html);

/* get device_code, user_code, intrerval and expires_in seconds 
 * for user authorisation in url with 
 * c_yandex_oauth_ask_token_in_interval
 * %device_name - any non-NULL string */
void c_yandex_oauth_on_page(
		const char *client_id, 
		const char *device_name,  //device name - any
		void * user_data,
		void (*callback)(
			void * user_data,
			const char * device_code,
			const char * user_code,
			const char * url,
			int interval,
			int expires_in,
			const char * error
			)
		);

/* ask periodicaly yandex server for token 
 * to stop function execute - return non-zero in callback*/ 
void c_yandex_oauth_ask_token_in_interval(
		const char *device_code, 
		const char *client_id,    //id of application in Yandex
		const char *client_secret,//secret of application in Yandex
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
	);

/* get token from yandex server
 * %device_name - any non-NULL string */
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
		);

#ifdef __cplusplus
}  /* end of the 'extern "C"' block */
#endif

#endif /* ifndef C_YANDEX_OAUTH_H */
// vim:ft=c
