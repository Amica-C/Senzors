# LoRaWAN Connection Guide

## Understanding LoRaWAN Gateway Connection

### Does the LoRaWAN module automatically connect to a gateway?

**YES**, the connection process is automatic. When you call `LoRaWAN_Init()`, the module:

1. Initializes the LoRaWAN stack
2. **Automatically sends join requests** to the network using OTAA (Over-The-Air Activation)
3. Waits for a join accept response from the network server
4. If join fails, **automatically retries** until successful

You do NOT need to call a separate "connect" function. The join process happens automatically in the background as long as you call `LoRaWAN_Process()` regularly in your main loop.

### How do I check if the gateway connection is established?

Use the `LoRaWAN_IsJoined()` function:

```c
if (LoRaWAN_IsJoined())
{
    // Connection established - you can now send data
    LoRaWAN_Send(myBuffer, bufferLen, 2, false);
}
else
{
    // Still joining - wait and keep calling LoRaWAN_Process()
}
```

For more detailed status information, use `LoRaWAN_GetConnectionStatus()`:

```c
const char* status = LoRaWAN_GetConnectionStatus();
// Returns: "Not initialized", "Joining...", or "Joined"
```

### Can I detect or scan for gateways in range?

**NO** - LoRaWAN protocol does NOT support gateway detection or scanning. Here's why:

#### How LoRaWAN Actually Works:

1. **Device sends blind join requests**
   - Your device transmits join requests on configured frequency channels
   - It doesn't know if any gateway is listening
   - It doesn't need to know which specific gateway receives the message

2. **Any gateway in range forwards the message**
   - Multiple gateways may receive your join request (this is good!)
   - Each gateway forwards it to the network server
   - The network server decides whether to accept the join request
   - This redundancy improves reliability

3. **Network server responds via any available gateway**
   - If your credentials (JoinEUI, DevEUI, AppKey) are valid
   - The network server sends a join accept message
   - Any gateway can deliver this response to your device

4. **Connection is established at network level**
   - You don't connect to a specific gateway
   - You connect to the **network** (represented by the network server)
   - Your messages may be received by different gateways each time

#### Why You Don't Need Gateway Detection:

- **Automatic routing**: Messages automatically find available gateways
- **Multiple gateways**: Several gateways can hear you simultaneously (improves reliability)
- **Transparent operation**: You don't care which gateway is used
- **Network decides**: The network server chooses the best gateway for downlinks

### Where in the code does connection happen?

The join process is initiated in **lorawan_app.c**, inside `LoRaWAN_Init()`:

```c
void LoRaWAN_Init(const lorawan_otaa_keys_t *otaa)
{
    // ... initialization code ...
    
    // This line automatically starts the join process:
    LmHandlerJoin(ACTIVATION_TYPE_OTAA, true);
    
    // ... rest of initialization ...
}
```

The join status is tracked through callbacks in `OnJoinRequest()`:

```c
static void OnJoinRequest(LmHandlerJoinParams_t *params)
{
    if (params->Status == LORAMAC_HANDLER_SUCCESS)
    {
        s_joined = true;  // Connection established!
        APP_LOG(TS_ON, VLEVEL_M, "LoRaWAN: Joined");
    }
    else
    {
        s_joined = false;
        APP_LOG(TS_ON, VLEVEL_M, "LoRaWAN: Join failed, retry...");
        LmHandlerJoin(ACTIVATION_TYPE_OTAA, true);  // Automatic retry
    }
}
```

## Complete Usage Example

```c
#include "lorawan_app.h"

int main(void)
{
    // Initialize hardware...
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    
    // Set LoRaWAN credentials (get these from your network server)
    uint8_t joinEui[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t appKey[16] = { /* your 16-byte app key */ };
    LoRaWAN_SetJoinCredentials(joinEui, appKey);
    
    // Initialize LoRaWAN - this AUTOMATICALLY starts join process
    LoRaWAN_Init(NULL);  // NULL = use defaults (DevEUI from MCU ID)
    
    printf("LoRaWAN initialized, joining network...\n");
    
    // Main loop
    while (1)
    {
        // REQUIRED: Process LoRaWAN events (handles join, send, receive)
        LoRaWAN_Process();
        
        // Check connection status
        const char* status = LoRaWAN_GetConnectionStatus();
        
        if (LoRaWAN_IsJoined())
        {
            // Connected! Can send data
            uint8_t sensorData[] = { 0x01, 0x23, 0x45 };
            int result = LoRaWAN_Send(sensorData, sizeof(sensorData), 2, false);
            
            if (result == 0)
            {
                printf("Data sent successfully!\n");
            }
            
            HAL_Delay(60000);  // Wait 1 minute before next send
        }
        else
        {
            // Not connected yet, show status
            printf("Status: %s\n", status);
            HAL_Delay(1000);
        }
    }
}
```

## Troubleshooting

### Join process never completes

**Possible causes:**

1. **No gateway in range**
   - Solution: Move closer to a gateway, or deploy your own gateway
   
2. **Wrong credentials**
   - Check DevEUI, JoinEUI (AppEUI), and AppKey match your network server configuration
   - DevEUI is auto-generated from MCU ID - make sure it's registered in your network
   
3. **Not calling LoRaWAN_Process()**
   - Solution: Must call `LoRaWAN_Process()` regularly in main loop
   
4. **Wrong region configuration**
   - Check `lorawan_conf.h` - ensure `REGION_EU868` (or your region) is defined
   - Verify `LORAWAN_REGION` matches your geographical location

5. **Network server not configured**
   - Your device must be registered in the network server
   - Check device is activated for OTAA mode
   - Verify network server is operational

### How long does join take?

- **Typical time**: 5-30 seconds in good conditions
- **With poor signal**: Can take several minutes
- **Automatic retries**: Will keep trying until successful
- **No timeout**: Join attempts continue indefinitely

### Can I force a rejoin?

Yes, call `LmHandlerJoin(ACTIVATION_TYPE_OTAA, true)` directly, but this is rarely needed as automatic retry handles failures.

## Summary

✅ **Connection is automatic** - starts when you call `LoRaWAN_Init()`  
✅ **Check status with** `LoRaWAN_IsJoined()` or `LoRaWAN_GetConnectionStatus()`  
❌ **Cannot scan for gateways** - LoRaWAN doesn't work that way  
✅ **Just wait for join** - will succeed if gateway in range with valid credentials  
✅ **Must call** `LoRaWAN_Process()` regularly for everything to work  

## API Reference

### Initialization Functions

- `LoRaWAN_SetJoinCredentials()` - Set JoinEUI and AppKey before init
- `LoRaWAN_Init()` - Initialize and automatically start join process
- `LoRaWAN_Process()` - **Must call regularly** in main loop

### Status Functions

- `LoRaWAN_IsJoined()` - Returns true if connected
- `LoRaWAN_GetConnectionStatus()` - Returns detailed status string

### Communication Functions

- `LoRaWAN_Send()` - Send uplink data (only works after joined)
- `LoRaWAN_RegisterRxCallback()` - Register callback for downlink messages

### Class Management Functions

- `LoRaWAN_RequestClass()` - Switch between Class A/B/C
- `LoRaWAN_GetClass()` - Get current device class
