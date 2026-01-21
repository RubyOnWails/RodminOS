#include "bluetooth.h"
#include "../kernel/kernel.h"

static bt_context_t bt_ctx;
static bt_device_t paired_devices[MAX_BT_DEVICES];
static uint32_t device_count = 0;

int init_bluetooth_stack(void) {
    if(detect_bluetooth_hardware() != 0) return -1;
    
    init_hci_layer();
    init_l2cap_layer();
    init_sdp_layer();
    init_rfcomm_layer();
    init_bt_profiles();
    
    bt_ctx.state = BT_STATE_READY;
    kprintf("Bluetooth stack initialized\n");
    return 0;
}

int bt_scan_devices(bt_device_info_t* devices, uint32_t* count) {
    bt_inquiry_t inquiry;
    inquiry.length = 10;
    inquiry.num_responses = 0;
    
    send_hci_command(HCI_INQUIRY, &inquiry, sizeof(inquiry));
    
    wait_for_inquiry_complete();
    
    *count = inquiry.num_responses;
    for(uint32_t i = 0; i < *count; i++) {
        devices[i] = inquiry.responses[i];
    }
    
    return 0;
}

int bt_pair_device(bt_address_t* addr, const char* pin) {
    if(device_count >= MAX_BT_DEVICES) return -1;
    
    bt_device_t* device = &paired_devices[device_count];
    memcpy(&device->address, addr, sizeof(bt_address_t));
    
    if(perform_pairing(device, pin) != 0) return -1;
    
    device->paired = true;
    device->connected = false;
    device_count++;
    
    return 0;
}

int bt_connect_device(bt_address_t* addr) {
    bt_device_t* device = find_paired_device(addr);
    if(!device) return -1;
    
    bt_connection_t conn;
    conn.handle = allocate_connection_handle();
    memcpy(&conn.remote_addr, addr, sizeof(bt_address_t));
    
    if(establish_acl_connection(&conn) != 0) return -1;
    
    device->connected = true;
    device->connection_handle = conn.handle;
    
    return 0;
}

void init_bt_profiles(void) {
    init_hid_profile();
    init_a2dp_profile();
    init_hfp_profile();
    init_opp_profile();
}

int init_hid_profile(void) {
    register_sdp_service(SDP_SERVICE_HID, hid_service_record);
    register_l2cap_psm(L2CAP_PSM_HID_CONTROL, handle_hid_control);
    register_l2cap_psm(L2CAP_PSM_HID_INTERRUPT, handle_hid_interrupt);
    return 0;
}

void handle_hid_control(l2cap_channel_t* channel, uint8_t* data, size_t len) {
    hid_message_t* msg = (hid_message_t*)data;
    
    switch(msg->type) {
        case HID_HANDSHAKE:
            handle_hid_handshake(channel, msg);
            break;
        case HID_CONTROL:
            handle_hid_control_msg(channel, msg);
            break;
        case HID_GET_REPORT:
            handle_hid_get_report(channel, msg);
            break;
        case HID_SET_REPORT:
            handle_hid_set_report(channel, msg);
            break;
    }
}

void handle_hid_interrupt(l2cap_channel_t* channel, uint8_t* data, size_t len) {
    hid_report_t* report = (hid_report_t*)data;
    
    switch(report->type) {
        case HID_REPORT_INPUT:
            process_input_report(channel, report);
            break;
        case HID_REPORT_OUTPUT:
            process_output_report(channel, report);
            break;
        case HID_REPORT_FEATURE:
            process_feature_report(channel, report);
            break;
    }
}

void process_input_report(l2cap_channel_t* channel, hid_report_t* report) {
    bt_device_t* device = find_device_by_channel(channel);
    if(!device) return;
    
    if(device->device_type == BT_DEVICE_KEYBOARD) {
        process_keyboard_report(report);
    } else if(device->device_type == BT_DEVICE_MOUSE) {
        process_mouse_report(report);
    } else if(device->device_type == BT_DEVICE_GAMEPAD) {
        process_gamepad_report(report);
    }
}