# Считаем количество слов в тексте

- `frequencyDictMmap` - используем filemapping для чтения файла и std::unordered_map
- `frequencyDictStream` - используем fstream из stl и std::unordered_map
- `frequencyDictStreamTrie` - используем fstream и наивную реализацию префиксного дерева

# Сборка примера

```console
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=../install ../avito
cmake --build . --target install
```
