#include <linphone/core.h>
#include <unistd.h>

int main() {
    // 1. Initialize Core
    LinphoneCore *lc = linphone_factory_create_core(linphone_factory_get(), NULL, NULL, NULL);

    // 2. Hardware Configuration
    // Disable video, but LEAVE AUDIO ENABLED to use the physical microphone passed through via Docker
    linphone_core_enable_video_capture(lc, FALSE);
    linphone_core_enable_video_display(lc, FALSE);
    
    // 3. Prevent std::logic_error crashes
    // Linphone's C++ backend throws a fatal error if passed NULL for databases. We use empty strings instead.
    linphone_core_set_chat_database_path(lc, "");
    linphone_core_set_call_logs_database_path(lc, "");

    // 4. Authentication
    // Replace with your Username, Password, and SIP Domain
    LinphoneAuthInfo *auth = linphone_auth_info_new(
        "your_username", NULL, "your_password", NULL, NULL, "sip.linphone.org"
    );
    linphone_core_add_auth_info(lc, auth);
    linphone_auth_info_unref(auth);

    // 5. Network Routing & Proxy
    LinphoneProxyConfig *proxy = linphone_core_create_proxy_config(lc);
    LinphoneAddress *addr = linphone_address_new("sip:your_username@sip.linphone.org");
    linphone_proxy_config_set_identity_address(proxy, addr);
    linphone_proxy_config_set_server_addr(proxy, "sip:sip.linphone.org;transport=tls");
    linphone_proxy_config_enable_register(proxy, TRUE);
    
    linphone_core_add_proxy_config(lc, proxy);
    linphone_core_set_default_proxy_config(lc, proxy);
    linphone_address_unref(addr);

    // 6. Wait for TLS Handshake and Server Registration
    for(int i = 0; i < 120; i++) {
        linphone_core_iterate(lc);
        usleep(50000); 
    }

    // 7. Initiate the Call
    // Replace with the destination SIP URI or Phone Number
    linphone_core_invite(lc, "sip:target_user@sip.linphone.org");

    // 8. Keep the application alive to process the audio stream
    while (1) {
        linphone_core_iterate(lc);
        usleep(50000);
    }

    return 0; 
}
