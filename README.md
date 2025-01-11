## Add external library
- execute cmds:
```bash
git submodule add https://github.com/lib_name.git external/lib_name

git submodule update --init
```

- update: CMakeLists.txt
```
add_subdirectory(external/lib_name)
```

