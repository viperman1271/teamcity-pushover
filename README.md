# TeamCity-Pushover [![Build Status](https://teamcity.mikefilion.com/app/rest/builds/buildType:(id:Root_Code_TeamcityPushoverGit)/statusIcon)]
This is an executable that will allow for parsing TeamCity information to push notifications through the Poushover servrice

## Information
This uses the [TeamCity REST API](https://www.jetbrains.com/help/teamcity/rest-api.html) to parse information from a build chain to produce a status for the build that triggered the associated task.

## Dependencies
This project uses [vcpkg](https://github.com/microsoft/vcpkg) package manager in addition to [cmake](https://cmake.org/) (minimum version 3.5). This is used to pull, configure, and build locally without directly linking their sources. A full list of needed dependencies follows;
* [CLI11](https://github.com/CLIUtils/CLI11)
* [CURL](https://curl.haxx.se/libcurl/) [[Github](https://github.com/curl/curl)]
* [yaml-cpp](https://github.com/jbeder/yaml-cpp)
* [tinyxml2](www.grinninglizard.com › tinyxml2) [[Github](https://github.com/leethomason/tinyxml2)]
* [json-c](https://github.com/json-c/json-c)

## How to compile
* Download and compile [vcpkg](https://github.com/microsoft/vcpkg)
* `vcpkg install cli11 curl yaml-cpp tinyxml2 json-c`
* `cmake .`
* `cmake --build .`

This produces the teamcity-pushover executable which is then usable.

## Supported platforms
Currently Windows and Linux are supported. Any additional UNIX based systems should work (*BSD, Mac, etc.)

# References
* [cpushover](https://github.com/cbjartli/cpushover) - A source of inspiration for 