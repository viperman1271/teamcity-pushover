# TeamCity-Pushover
This is an executable that will allow for parsing TeamCity information to push notifications through the Poushover servrice

## Information
This uses the [TeamCity REST API](https://www.jetbrains.com/help/teamcity/rest-api.html) to parse information from a build chain to produce a status for the build that triggered the associated task.

## Dependencies
This project uses [vcpkg](https://github.com/microsoft/vcpkg) package manager. This is used to pull, configure, and build locally without directly linking their sources. A full list of needed dependencies follows;
* [CLI11](https://github.com/CLIUtils/CLI11)
* [CURL](https://curl.haxx.se/libcurl/) [[Github](https://github.com/curl/curl)]
* [yaml-cpp](https://github.com/jbeder/yaml-cpp)
* [tinyxml2](www.grinninglizard.com › tinyxml2) [[Github](https://github.com/leethomason/tinyxml2)]
* [json-c](https://github.com/json-c/json-c)

# References
* [cpushover](https://github.com/cbjartli/cpushover) - A source of inspiration for 