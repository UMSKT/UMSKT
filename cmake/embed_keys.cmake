if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "embed_keys.cmake requires -DINPUT= and -DOUTPUT=")
endif()

file(READ "${INPUT}" KEYS)
file(WRITE "${OUTPUT}" [=[extern const char umskt_keys_json[] = R"UMSKTJSON(
]=] "${KEYS}" [=[
)UMSKTJSON";
extern const int umskt_keys_json_size = (int)(sizeof(umskt_keys_json) - 1);
]=])
