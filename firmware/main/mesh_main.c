/* Mesh Internal Communication Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <sys/time.h>

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mesh.h"
#include "esp_mesh_internal.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "endian.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "include/mesh_main.h"
#include "include/ms_crypto.h"
#include "include/ms_network.h"
#include "include/ms_gpio.h"

/*******************************************************
 *                Variable Definitions
 *******************************************************/
static const char *LOG_TAG = "ms-project-main";

static uint8_t task_meshservice_tx_buf[TX_SIZE] = {
    0,
};
static uint8_t task_meshservice_rx_buf[RX_SIZE] = {
    0,
};

static bool is_running = true;
static bool is_mesh_connected = false;
static bool is_ds_connected = false;

static bool is_task_meshservice_started = false;

static mesh_addr_t mesh_parent_addr;
static int mesh_layer = -1;
static esp_netif_t *netif_sta = NULL;

static flash_data_t fdata;

static bool gotSTAIP = false;

static mesh_addr_t rootAddress;
static mesh_addr_t staAddress, softAPAddress;

/*******************************************************
 *                Function Declarations
 *******************************************************/

/*******************************************************
 *                Function Definitions
 *******************************************************/

void startDHCPC()
{
    if (esp_mesh_is_root())
    {
        esp_netif_dhcpc_start(netif_sta);
    }
}

void stopDHCPC()
{
    esp_netif_dhcp_status_t dhcpc_status;
    esp_netif_dhcpc_get_status(netif_sta, &dhcpc_status);
    if (!esp_mesh_is_root() && dhcpc_status == ESP_NETIF_DHCP_STARTED)
    {
        esp_netif_dhcpc_stop(netif_sta);
    }
}

int setCurrentTime(int64_t timestamp_sec, int64_t timestamp_usec)
{
    struct timeval timestamp;
    timestamp.tv_sec = timestamp_sec;
    timestamp.tv_usec = timestamp_usec;

    if (settimeofday(&timestamp, NULL) == 0)
        return ESP_OK;
    else
        return -1;
}

int sameAddress(mesh_addr_t *a, mesh_addr_t *b)
{
    uint8_t *addrA = a->addr;
    uint8_t *addrB = b->addr;

    for (int i = 0; i < 6; i++)
        if (addrA[i] != addrB[i])
            return -1;

    return ESP_OK;
}

void esp_task_meshservice(void *arg)
{
    app_config_t *config = &(fdata.config);

    esp_err_t err;
    mesh_addr_t from;
    mesh_data_t data;
    int flag = 0;
    data.data = task_meshservice_rx_buf;
    data.size = RX_SIZE;
    is_running = true;

    while (is_running)
    {
        //Devo ricevere i pacchetti per l'esterno
        err = esp_mesh_recv(&from, &data, 10, &flag, NULL, 0);

        if (esp_mesh_is_root()) //Sono il root: se ricevo il frame lo devo inoltrare al server.
        {
            //Controllo che sia un mio figlio diretto.
            //Aggrego il dato e aspetto di inviare tutto con la mia frequenza.

            //Se ricevo da me stesso passo.
            if (sameAddress(&staAddress, &from) == ESP_OK)
                continue;

            //Andato in timeout continuiamo
            if (err != ESP_ERR_MESH_TIMEOUT)
            {
                if (err != ESP_OK || data.size < sizeof(app_frame_t))
                {
                    ESP_LOGE(LOG_TAG, "(Master)Recv failed. err: %d, size: %d from " MACSTR ", root: " MACSTR, err, data.size, MAC2STR(from.addr), MAC2STR(rootAddress.addr));
                }
                else
                {
                    //Ricevuto: devo inoltrarlo al server.
                    memcpy(task_meshservice_tx_buf, task_meshservice_rx_buf, sizeof(app_frame_t));
                    sendUDP(task_meshservice_tx_buf, sizeof(app_frame_t), config->server_ip, config->server_port);
                    ESP_LOGI(LOG_TAG, "Inoltro al server un frame da parte di " MACSTR, MAC2STR(from.addr));
                }
            }
        }else{//Non sono un root, ma un nodo intermedio.
            //Controllo che sia un mio figlio diretto.
            //Aggrego il dato e aspetto di inviare tutto con la mia frequenza.

            //Se ricevo da me stesso passo.
            if (sameAddress(&staAddress, &from) == ESP_OK)
                continue;

            //Andato in timeout continuiamo
            if (err != ESP_ERR_MESH_TIMEOUT)
            {

            }

        }
        else
        { //Sono un nodo normale: ricevo la richiesta e la elaboro
            //Andato in timeout continuiamo
            if (err != ESP_ERR_MESH_TIMEOUT)
            {
                // size_t len = createSensorPacket(data, task_meshservice_tx_buf, TX_SIZE);

                // mesh_data_t data;
                // data.data = task_meshservice_tx_buf;
                // data.size = len;
                // data.proto = MESH_PROTO_BIN;
                // data.tos = MESH_TOS_P2P;

                // //Invio al server se possibile (la root fa da bridge).
                // err = esp_mesh_send(NULL, &data, 0, NULL, 0);

                // if (err != ESP_OK)
                // {
                //     ESP_LOGE(LOG_TAG,
                //                 "Failed to send packet to root: %d", err);
                // }
            }
        }


    }

    is_task_meshservice_started = false;
    vTaskDelete(NULL);
}


esp_err_t esp_start_tasks(void)
{
    if (!is_task_meshservice_started)
    {
        is_task_meshservice_started = true;
        xTaskCreate(esp_task_meshservice, "TSKMSH", 3072, NULL, 5, NULL);
    }

    return ESP_OK;
}

void mesh_event_handler(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data)
{
    mesh_addr_t id = {
        0,
    };
    static uint16_t last_layer = 0;

    switch (event_id)
    {
    case MESH_EVENT_STARTED:
    {
        esp_mesh_get_id(&id);
        ESP_LOGI(LOG_TAG, "<MESH_EVENT_MESH_STARTED>ID:" MACSTR "", MAC2STR(id.addr));
        is_mesh_connected = false;
        is_ds_connected = false;
        gotSTAIP = false;
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_STOPPED:
    {
        ESP_LOGI(LOG_TAG, "<MESH_EVENT_STOPPED>");
        is_mesh_connected = false;
        is_ds_connected = false;
        gotSTAIP = false;
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_CHILD_CONNECTED:
    {
        mesh_event_child_connected_t *child_connected = (mesh_event_child_connected_t *)event_data;
        ESP_LOGI(LOG_TAG, "<MESH_EVENT_CHILD_CONNECTED>aid:%d, " MACSTR "",
                 child_connected->aid,
                 MAC2STR(child_connected->mac));
    }
    break;
    case MESH_EVENT_CHILD_DISCONNECTED:
    {
        mesh_event_child_disconnected_t *child_disconnected = (mesh_event_child_disconnected_t *)event_data;
        ESP_LOGI(LOG_TAG, "<MESH_EVENT_CHILD_DISCONNECTED>aid:%d, " MACSTR "",
                 child_disconnected->aid,
                 MAC2STR(child_disconnected->mac));
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_ADD:
    {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        ESP_LOGW(LOG_TAG, "<MESH_EVENT_ROUTING_TABLE_ADD>add %d, new:%d, layer:%d",
                 routing_table->rt_size_change,
                 routing_table->rt_size_new, mesh_layer);
    }
    break;
    case MESH_EVENT_ROUTING_TABLE_REMOVE:
    {
        mesh_event_routing_table_change_t *routing_table = (mesh_event_routing_table_change_t *)event_data;
        ESP_LOGW(LOG_TAG, "<MESH_EVENT_ROUTING_TABLE_REMOVE>remove %d, new:%d, layer:%d",
                 routing_table->rt_size_change,
                 routing_table->rt_size_new, mesh_layer);
    }
    break;
    case MESH_EVENT_NO_PARENT_FOUND:
    {
        mesh_event_no_parent_found_t *no_parent = (mesh_event_no_parent_found_t *)event_data;
        ESP_LOGI(LOG_TAG, "<MESH_EVENT_NO_PARENT_FOUND>scan times:%d",
                 no_parent->scan_times);
    }
    /* TODO handler for the failure */
    break;
    case MESH_EVENT_PARENT_CONNECTED:
    {
        mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
        esp_mesh_get_id(&id);
        mesh_layer = connected->self_layer;
        memcpy(&mesh_parent_addr.addr, connected->connected.bssid, 6);
        ESP_LOGI(LOG_TAG,
                 "<MESH_EVENT_PARENT_CONNECTED>layer:%d-->%d, parent:" MACSTR "%s, ID:" MACSTR ", duty:%d",
                 last_layer, mesh_layer, MAC2STR(mesh_parent_addr.addr),
                 esp_mesh_is_root() ? "<ROOT>" : (mesh_layer == 2) ? "<layer2>"
                                                                   : "",
                 MAC2STR(id.addr), connected->duty);
        last_layer = mesh_layer;
        is_mesh_connected = true;
        startDHCPC();
        esp_start_tasks();
    }
    break;
    case MESH_EVENT_PARENT_DISCONNECTED:
    {
        mesh_event_disconnected_t *disconnected = (mesh_event_disconnected_t *)event_data;
        ESP_LOGI(LOG_TAG,
                 "<MESH_EVENT_PARENT_DISCONNECTED>reason:%d",
                 disconnected->reason);
        is_mesh_connected = false;
        gotSTAIP = false;
        mesh_layer = esp_mesh_get_layer();
    }
    break;
    case MESH_EVENT_LAYER_CHANGE:
    {
        mesh_event_layer_change_t *layer_change = (mesh_event_layer_change_t *)event_data;
        mesh_layer = layer_change->new_layer;
        ESP_LOGI(LOG_TAG, "<MESH_EVENT_LAYER_CHANGE>layer:%d-->%d%s",
                 last_layer, mesh_layer,
                 esp_mesh_is_root() ? "<ROOT>" : (mesh_layer == 2) ? "<layer2>"
                                                                   : "");
        last_layer = mesh_layer;
    }
    break;
    case MESH_EVENT_ROOT_ADDRESS:
    {
        mesh_event_root_address_t *root_addr = (mesh_event_root_address_t *)event_data;
        ESP_LOGI(LOG_TAG, "<MESH_EVENT_ROOT_ADDRESS>root address:" MACSTR "",
                 MAC2STR(root_addr->addr));

        memcpy(&rootAddress, root_addr, 6);
    }
    break;
    case MESH_EVENT_VOTE_STARTED:
    {
        mesh_event_vote_started_t *vote_started = (mesh_event_vote_started_t *)event_data;
        ESP_LOGI(LOG_TAG,
                 "<MESH_EVENT_VOTE_STARTED>attempts:%d, reason:%d, rc_addr:" MACSTR "",
                 vote_started->attempts,
                 vote_started->reason,
                 MAC2STR(vote_started->rc_addr.addr));
    }
    break;
    case MESH_EVENT_VOTE_STOPPED:
    {
        ESP_LOGI(LOG_TAG, "<MESH_EVENT_VOTE_STOPPED>");
        break;
    }
    case MESH_EVENT_ROOT_SWITCH_REQ:
    {
        mesh_event_root_switch_req_t *switch_req = (mesh_event_root_switch_req_t *)event_data;
        ESP_LOGI(LOG_TAG,
                 "<MESH_EVENT_ROOT_SWITCH_REQ>reason:%d, rc_addr:" MACSTR "",
                 switch_req->reason,
                 MAC2STR(switch_req->rc_addr.addr));
    }
    break;
    case MESH_EVENT_ROOT_SWITCH_ACK:
    {
        /* new root */
        mesh_layer = esp_mesh_get_layer();
        esp_mesh_get_parent_bssid(&mesh_parent_addr);

        ESP_LOGI(LOG_TAG, "<MESH_EVENT_ROOT_SWITCH_ACK>layer:%d, parent:" MACSTR "", mesh_layer, MAC2STR(mesh_parent_addr.addr));
    }
    break;
    case MESH_EVENT_TODS_STATE:
    {
        mesh_event_toDS_state_t *toDs_state = (mesh_event_toDS_state_t *)event_data;
        is_ds_connected = *toDs_state == MESH_TODS_REACHABLE;

        ESP_LOGI(LOG_TAG, "<MESH_EVENT_TODS_REACHABLE>state:%d", *toDs_state);
    }
    break;
    case MESH_EVENT_ROOT_ASKED_YIELD:
    {
        mesh_event_root_conflict_t *root_conflict = (mesh_event_root_conflict_t *)event_data;
        ESP_LOGI(LOG_TAG,
                 "<MESH_EVENT_ROOT_ASKED_YIELD>" MACSTR ", rssi:%d, capacity:%d",
                 MAC2STR(root_conflict->addr),
                 root_conflict->rssi,
                 root_conflict->capacity);
    }
    break;
    case MESH_EVENT_CHANNEL_SWITCH:
    {
        mesh_event_channel_switch_t *channel_switch = (mesh_event_channel_switch_t *)event_data;
        ESP_LOGI(LOG_TAG, "<MESH_EVENT_CHANNEL_SWITCH>new channel:%d", channel_switch->channel);
    }
    break;
    case MESH_EVENT_SCAN_DONE:
    {
        mesh_event_scan_done_t *scan_done = (mesh_event_scan_done_t *)event_data;
        ESP_LOGI(LOG_TAG, "<MESH_EVENT_SCAN_DONE>number:%d",
                 scan_done->number);
    }
    break;
    case MESH_EVENT_NETWORK_STATE:
    {
        mesh_event_network_state_t *network_state = (mesh_event_network_state_t *)event_data;
        ESP_LOGI(LOG_TAG, "<MESH_EVENT_NETWORK_STATE>is_rootless:%d",
                 network_state->is_rootless);
    }
    break;
    case MESH_EVENT_STOP_RECONNECTION:
    {
        ESP_LOGI(LOG_TAG, "<MESH_EVENT_STOP_RECONNECTION>");
    }
    break;
    case MESH_EVENT_FIND_NETWORK:
    {
        mesh_event_find_network_t *find_network = (mesh_event_find_network_t *)event_data;
        ESP_LOGI(LOG_TAG, "<MESH_EVENT_FIND_NETWORK>new channel:%d, router BSSID:" MACSTR "",
                 find_network->channel, MAC2STR(find_network->router_bssid));
    }
    break;
    case MESH_EVENT_ROUTER_SWITCH:
    {
        mesh_event_router_switch_t *router_switch = (mesh_event_router_switch_t *)event_data;
        ESP_LOGI(LOG_TAG, "<MESH_EVENT_ROUTER_SWITCH>new router:%s, channel:%d, " MACSTR "",
                 router_switch->ssid, router_switch->channel, MAC2STR(router_switch->bssid));
    }
    break;
    case MESH_EVENT_PS_PARENT_DUTY:
    {
        mesh_event_ps_duty_t *ps_duty = (mesh_event_ps_duty_t *)event_data;
        ESP_LOGI(LOG_TAG, "<MESH_EVENT_PS_PARENT_DUTY>duty:%d", ps_duty->duty);
    }
    break;
    case MESH_EVENT_PS_CHILD_DUTY:
    {
        mesh_event_ps_duty_t *ps_duty = (mesh_event_ps_duty_t *)event_data;
        ESP_LOGI(LOG_TAG, "<MESH_EVENT_PS_CHILD_DUTY>cidx:%d, " MACSTR ", duty:%d", ps_duty->child_connected.aid - 1,
                 MAC2STR(ps_duty->child_connected.mac), ps_duty->duty);
    }
    break;
    default:
        ESP_LOGI(LOG_TAG, "unknown id:%d", event_id);
        break;
    }
}

void ip_event_handler(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(LOG_TAG, "<IP_EVENT_STA_GOT_IP>IP:" IPSTR, IP2STR(&event->ip_info.ip));
    gotSTAIP = true;

    app_config_t *config = &(fdata.config);

    if (esp_mesh_is_root())
    {
        //creazione socket
        createSocket();
        //bindSocket(localPort?);
    }
}

void wifi_event_handler(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGW(LOG_TAG, "<WIFI_EVENT_AP_STADISCONNECTED>");
    gotSTAIP = false;
    closeSocket();
}

void app_main(void)
{
    //Inizializzazione flash fake
    fdata.config.module_id = 2;
    fdata.config.wifi_channel = 1;
    fdata.config.server_port = 11412;
    fdata.config.task_meshservice_delay_millis = 500;
    fdata.config.mesh_send_timeout_millis = 1000;

    for (int i = 0; i < 6; i++)
        fdata.config.mesh_id[i] = 0x71 + i;

    memcpy(&fdata.config.server_ip, "192.168.1.63", 13);
    memcpy(&fdata.config.mesh_pwd, "mastrogatto", 12);
    memcpy(&fdata.config.station_ssid, "WIFI_HOME_R", 12);
    memcpy(&fdata.config.station_pwd, "", 0);
    memcpy(&fdata.config.key_aes, "d5ff2c84db72f9039580bf5c45cc28b5", 32);
    memcpy(&fdata.config.key_hmac, "dd1aafdf54893f1481885e2b7af5f31f",32);
    
    //Init Crypto
    ESP_ERROR_CHECK(initHMAC(fdata.config.key_hmac));
    ESP_ERROR_CHECK(initAES(fdata.config.key_aes, fdata.config.iv_aes));

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    // if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    // {
    //     // NVS partition was truncated and needs to be erased
    //     // Retry nvs_flash_init
    //     ESP_LOGW(LOG_TAG, "Flash truncated: erasing...");
    //     ESP_ERROR_CHECK(nvs_flash_erase());
    //     err = nvs_flash_init();
    // }
    ESP_ERROR_CHECK(err);

    /*  tcpip initialization */
    ESP_ERROR_CHECK(esp_netif_init());
    /*  event initialization */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    /*  create network interfaces for mesh (only station instance saved for further manipulation, soft AP instance ignored */
    ESP_ERROR_CHECK(esp_netif_create_default_wifi_mesh_netifs(&netif_sta, NULL));

    /*  wifi initialization */
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_STA, staAddress.addr));
    ESP_ERROR_CHECK(esp_wifi_get_mac(ESP_IF_WIFI_AP, softAPAddress.addr));

    /*  mesh initialization */
    ESP_ERROR_CHECK(esp_mesh_send_block_time(fdata.config.mesh_send_timeout_millis));
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));

    /*  set mesh topology */
    ESP_ERROR_CHECK(esp_mesh_set_topology(CONFIG_MESH_TOPOLOGY));

    /*  set mesh max layer according to the topology */
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(CONFIG_MESH_MAX_LAYER));
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    ESP_ERROR_CHECK(esp_mesh_set_xon_qsize(128));

    /* Enable mesh PS function */
    ESP_ERROR_CHECK(esp_mesh_enable_ps());
    /* better to increase the associate expired time, if a small duty cycle is set. */
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(60));
    /* better to increase the announce interval to avoid too much management traffic, if a small duty cycle is set. */
    ESP_ERROR_CHECK(esp_mesh_set_announce_interval(600, 3300));

    /* Disable mesh PS function */
    // ESP_ERROR_CHECK(esp_mesh_disable_ps());
    // ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));

    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();

    /* mesh ID */
    memcpy((uint8_t *)&cfg.mesh_id, fdata.config.mesh_id, 6);

    /* router */
    cfg.channel = CONFIG_MESH_CHANNEL;
    cfg.router.ssid_len = strlen(fdata.config.station_ssid);
    memcpy((uint8_t *)&cfg.router.ssid, fdata.config.station_ssid, cfg.router.ssid_len);
    memcpy((uint8_t *)&cfg.router.password, fdata.config.station_pwd,
           strlen(fdata.config.station_pwd));

    /* mesh softAP */
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(CONFIG_MESH_AP_AUTHMODE));
    cfg.mesh_ap.max_connection = CONFIG_MESH_AP_CONNECTIONS;
    cfg.mesh_ap.nonmesh_max_connection = CONFIG_MESH_NON_MESH_AP_CONNECTIONS;
    memcpy((uint8_t *)&cfg.mesh_ap.password, CONFIG_MESH_AP_PASSWD,
           strlen(CONFIG_MESH_AP_PASSWD));
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));

    /* mesh start */
    ESP_ERROR_CHECK(esp_mesh_start());

    ESP_LOGI(LOG_TAG, "mesh starts successfully, heap:%d, %s<%d>%s, ps:%d\n", esp_get_minimum_free_heap_size(),
             esp_mesh_is_root_fixed() ? "root fixed" : "root not fixed",
             esp_mesh_get_topology(), esp_mesh_get_topology() ? "(chain)" : "(tree)", esp_mesh_is_ps_enabled());
}
