# Freq
## Считаем количество слов в тексте и выводим в файл

Запуск программы: 
```console
./freq <input> <output>
```

## Сборка примера

```console
git clone https://github.com/TemplateVoid/freq.git
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH=../install ../freq
cmake --build . --target install
```
