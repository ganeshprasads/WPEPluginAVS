<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.The_Alexa_Voice_Service_Headless_Client"></a>
# The Alexa Voice Service Headless Client

**Version: 1.0**

**Status: :black_circle::white_circle::white_circle:**

AVS plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the AVS plugin. It includes detailed specification of its configuration and methods provided.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers on the interface described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a name="head.Acronyms,_Abbreviations_and_Terms"></a>
## Acronyms, Abbreviations and Terms

The table below provides and overview of acronyms used in this document and their definitions.

| Acronym | Description |
| :-------- | :-------- |
| <a name="acronym.API">API</a> | Application Programming Interface |
| <a name="acronym.HTTP">HTTP</a> | Hypertext Transfer Protocol |
| <a name="acronym.JSON">JSON</a> | JavaScript Object Notation; a data interchange format |
| <a name="acronym.JSON-RPC">JSON-RPC</a> | A remote procedure call protocol encoded in JSON |

The table below provides and overview of terms and abbreviations used in this document and their definitions.

| Term | Description |
| :-------- | :-------- |
| <a name="term.callsign">callsign</a> | The name given to an instance of a plugin. One plugin can be instantiated multiple times, but each instance the instance name, callsign, must be unique. |

<a name="head.References"></a>
## References

| Ref ID | Description |
| :-------- | :-------- |
| <a name="ref.HTTP">[HTTP](http://www.w3.org/Protocols)</a> | HTTP specification |
| <a name="ref.JSON-RPC">[JSON-RPC](https://www.jsonrpc.org/specification)</a> | JSON-RPC 2.0 specification |
| <a name="ref.JSON">[JSON](http://www.json.org/)</a> | JSON specification |
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | Thunder API Reference |

<a name="head.Description"></a>
# Description

The Alexa Voice Service Headless Client serves as a personal assistant

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *WebKitBrowser*) |
| classname | string | Class name: *AVS* |
| locator | string | Library name: *libWPEFrameworkAVS.so* |
| autostart | boolean | Determines if the plugin is to be started automatically along with the framework |
| configuration | object | <sup>*(optional)*</sup>  |
| configuration.configfile | string | The main configutation file path |
| configuration?.pathtokwdinput | string | <sup>*(optional)*</sup> Path to directory with Key Word Detection input files |
| configuration.logLevel | string | The main configutation file path |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the AVS plugin:

AVS interface methods:

| Method | Description |
| :-------- | :-------- |
| [tap](#method.tap) | Start tap to talk |

<a name="method.tap"></a>
## *tap <sup>method</sup>*

Start tap to talk.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | null | Always null |

### Errors

| Code | Message | Description |
| :-------- | :-------- | :-------- |
| 2 | ```ERROR_UNAVAILABLE``` | AVS is unavailable |
| 12 | ```ERROR_INPROGRESS``` | There is already pending request |
| 1 | ```ERROR_GENERAL``` | Command failed to execute or does not exists |

### Example

#### Request

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "method": "AVS.1.tap"
}
```
#### Response

```json
{
    "jsonrpc": "2.0", 
    "id": 1234567890, 
    "result": null
}
```
