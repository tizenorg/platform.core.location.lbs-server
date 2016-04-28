# Fused Location
Fused Location is a service which provides a simplified way of accesing
device geolocation.

## Requirements
All requirements are available on current Tizen 3.0 images. To fully test
this service the GPS must be available on the target.

## Installation
Use GBS to build the package, and install it on a valid Tizen 3.0 target.

Note that some RPM packages available in the Tizen repository might not
be installed on your current target device. In that case you should install
them manually. Copies used during the build process are available in your
GBS-ROOT cache.

## Manual Testing
A temporary solution is to use two separate root shells on the target and
run following commands on each of them.

```
fused-location
```

```
fused-location-client
```

> Note: This is a temporary solution as stated in `/src/client/main.cpp`
>       fused-location-client will be replaced by a client library in
>       the future.

## Automatic Testing
Run:
```
fused-location-tests
```

The project uses Google Test Framework. A list of available options can
be obtained by running:
```
fused-location-tests --help
```

