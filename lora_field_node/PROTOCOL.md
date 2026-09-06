# Protocol Specification

## 1. Transport and Frame Structure

Transport: LoRa, SX1276, frequency 867.5 MHz.

Each message is sent as one binary LoRa packet.

Frame structure:

| Field | Size |
|---|---:|
| Version | 1 byte |
| Type | 1 byte |
| Sequence | 1 byte |
| Payload Length | 1 byte |
| Payload | N bytes |

General frame format:

`Version | Type | Sequence | Payload Length | Payload`

## 2. Frame Fields

| Offset | Size | Field | Type | Byte order |
|---:|---:|---|---|---|
| 0 | 1 byte | Version | uint8_t | N/A |
| 1 | 1 byte | Type | uint8_t | N/A |
| 2 | 1 byte | Sequence | uint8_t | N/A |
| 3 | 1 byte | Payload Length | uint8_t | N/A |
| 4 | N bytes | Payload | depends on message type | depends on payload |

## 3. Message Types and Payload

| Type | Name | Payload |
|---:|---|---|
| 0x01 | POST | 1-byte POST result bitmask |

### POST payload

POST payload size: 1 byte.

Bit mapping:

| Bit | Meaning |
|---:|---|
| 0 | Flash test failed |
| 1 | NVS test failed |
| 2 | OLED test failed |
| 3 | Radio test failed |

If all bits are 0, all POST checks passed successfully.

## 4. Protocol Versioning

The first byte of every packet contains the protocol version.

Current protocol version:

`0x01`

The protocol version is increased only when a change is not backward-compatible.

Examples of incompatible changes:
- changing the meaning of an existing field;
- changing the size or order of existing frame fields;
- changing an existing payload format so that an old receiver would parse it incorrectly.

Adding a new message type does not require a protocol version change if older receivers can safely ignore unknown message types.

## 5. Errors, ACK, Retry and Timeout

POST telemetry is currently sent without ACK.

For message types that require reliable delivery, the receiver should return an ACK containing the same Sequence value as the received packet.

If the sender does not receive an ACK within the timeout, it retries the transmission.

Recommended values:
- ACK timeout: 500 ms
- Maximum retries: 3

If all retries fail, the transmission is considered failed and the error is written to the log.

## 6. Example HEX Packet

Example POST packet:

`01 01 00 01 00`

Byte breakdown:

| Offset | HEX | Meaning |
|---:|---:|---|
| 0 | 0x01 | Protocol version = 1 |
| 1 | 0x01 | Message type = POST |
| 2 | 0x00 | Sequence = 0 |
| 3 | 0x01 | Payload length = 1 byte |
| 4 | 0x00 | POST result bitmask = all tests passed |

POST mask `0x00` means that Flash, NVS, OLED and Radio checks all passed.

## 7. Limits

- Radio: LoRa, SX1276.
- Operating frequency: 867.5 MHz.
- Maximum LoRa packet size: 255 bytes.
- Protocol header size: 4 bytes.
- Maximum payload size: 251 bytes.
- Payload Length is stored in one byte.
- POST telemetry is sent once after device startup.
- Transmission frequency for other message types must be limited according to the application requirements.
- Duty cycle must comply with the regulations for the frequency band and region where the device is used.