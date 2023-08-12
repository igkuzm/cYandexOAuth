/**
 * File              : YandexConnect.swift
 * Author            : Igor V. Sementsov <ig.kuzm@gmail.com>
 * Date              : 14.07.2022
 * Last Modified Date: 12.08.2023
 * Last Modified By  : Igor V. Sementsov <ig.kuzm@gmail.com>
 */

import SwiftUI

/*class SelfPointerYandexDiskView: NSObject {*/
    /*var selfptr: YandexDiskView?*/
/*}*/

struct YandexDiskView: View {
    @State var online = false
    @State var connected = false
    @State var isShowYDConnect = false
    @State var urlString = ""
    /*@State var selfPointer = SelfPointerYandexDiskView()*/
    var body: some View {
        VStack {
            if !online {
                ProgressView()
                .progressViewStyle(CircularProgressViewStyle())
                Text("Ждём подключения к интернету...")
            } else {
                if connected { 
                    HStack {
                        Text("Яндекс Диск подключен")
                    }
                } else {
                    HStack {
                        Text("Яндекс Диск не подключен")
                        Image(uiImage: UIImage(named: "yd-ind-error.png")!)
                    }
                    Button {
                        var err = strdup("")
                        if let url = c_yandex_oauth_code_url(CLIENTID){
                            let url = String(cString: url)
                            urlString = url
                        } else {
                            let error = String(cString: err!)
                            if error != "" {
                                print("\(error)")
                            }
                        }
                    } label: {
                        Text("Подключить")
                    }
                }
            }
        }
        .onAppear {
            checkInternet()
        }
        .onChange(of: urlString) { urlString in
            if (urlString != ""){
                isShowYDConnect = true
            }
        }        
        .onChange(of: isShowYDConnect) { isShowYDConnect in
            if (!isShowYDConnect){
                checkYDConnection()
            }
        }
        .sheet(isPresented: $isShowYDConnect) {
            YandexDiskConnectView(url: urlString, urlString: $urlString, isShowYDConnect: $isShowYDConnect)
        }        
    }
    func checkInternet() {
        Reachability(handler: { available in
            if (available){
                checkYDConnection()
                online = true
            }
        }) 
    }
    func checkYDConnection() {
        if let token = UserDefaults.standard.value(forKey: "YDToken") {
            var file = c_yd_file_t()
            var err = strdup("")
            if let token = (token as! NSString).utf8String {
                c_yandex_disk_file_info(token, "app:/", &file, &err)
                let error = String(cString: err!)
                if error == "" {
                    connected = true
                }
            }
        }
    }
}

struct YandexDiskConnectView: View {
    @State var url = ""
    @Binding var urlString: String
    @Binding var isShowYDConnect: Bool
    @State var code = ""
    @StateObject var model = WebViewModel()    
    var body: some View {
        ZStack {
            WebView(webView: model.webView)
            .onChange(of: model.isLoading) { isLoading in
                if !isLoading {
                   findVerificationCode() 
                }
            }
            
            if model.isLoading {
                ProgressView()
                    .progressViewStyle(CircularProgressViewStyle())
            }
        }
        .onAppear {
            loadData()
        }
    }
    func loadData() {
        //remove commas
        model.urlString = url
        model.loadUrl()
        urlString = ""
    }
    func findVerificationCode() {
        model.webView.evaluateJavaScript("document.body.innerHTML", completionHandler: { result, error in
            if let error = error{
                print("\(error)")
            }
            if let html = result as? String {
                print(html)
                if let html = (html as NSString).utf8String {
                    var err = strdup("")
                    if let code = c_yandex_oauth_code_from_html(html, &err){
                        let error = String(cString: err!)
                        if error != "" {
                            print("\(error)")
                        }                        
                        let verCode = String(cString: code)
                        print("VERIFICATION CODE: \(verCode)")
                        getToken(code: String(verCode)) 
                        isShowYDConnect = false
                        free(code)
                    } 
                }
            } 
        })
    }
    func getToken(code: String){
        let name = UIDevice.current.name
        if let code = (code as NSString).utf8String,
            let name = (name as NSString).utf8String {
            
            c_yandex_oauth_get_token(code, CLIENTID,
                                    CLIENTSECRET, name,
                                    nil, { data, token, exp, reftoken, error in
                if let error = error {
                    let error = String(cString: error)
                    print("Error: \(error)")
                }
                
                if let token = token {
                    stroybat_set_yandex_token(token);
                    let token = String(cString: token)
                    print("TOKEN: \(token)")
                    UserDefaults.standard.setValue(token, forKey: "YDToken")
                }

                return 0
            })
        }
    }
}
