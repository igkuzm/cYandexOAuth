# cYandexOAuth

C library for Yandex OAuth2 authorisation

```
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


```
