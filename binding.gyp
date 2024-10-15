{
  "targets":
  [
    {
      "target_name": "mmapIPC",
      "sources": [ "mmapIPC.cpp" ],
      "include_dirs": [ "node_modules/node-addon-api" ],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS", "NODE_ADDON_API_ENABLE_TYPE_CHECK_ON_AS" ]
    }
  ]
}


# "actions": [
#             {
#                 "action_name": "print_variables",
#                 "action": [ "echo", "|-------------------------------------------> <(module_root_dir)", ],

#                 "inputs": [],
#                 "outputs": [ "report.cc" ],
#             }
#         ],
