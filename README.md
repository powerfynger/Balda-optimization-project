# Оптимизированная версия игры "Балда"
## Что это такое?
"Балда", определение из Википедии, это: 
>Лингвистическая настольная игра для 2—4 игроков, в которой необходимо составлять слова с помощью букв, добавляемых определённым образом на квадратное игровое поле. 

### Ключевые особенности:
- ##### Каждая клетка содержит только одну букву, каждая буква в составленном слове приносит игроку одно очко
- ##### Слово должно содержаться хотя бы в одном толковом или энциклопедическом словаре(запрещены аббревиатуры, слова с  уменьшительно-ласкательными суффиксами и т.д.)
- ##### Слова в одной игре повторяться не могут, даже если это омонимы

Всего было реализовано три режима игры:
1. Дзен -- Одиночный режим с простой постановкой слов. Если пользователь не может придумать слово, то он может воспользоваться подсказкой и слово будет подобрано автоматически. Оптимизации, которые будут описаны далее лишь косвенно повлияют на данный режим.
2. Дуэль -- Вариант игры с локальным соперником. Два игрока по очереди ставят буквы. Если же слово не может быть найдено, то есть возможность пропустить ход. В случае, если ход был пропущен три раза, выигрывает тот игрок, который набрал большее количество очков. Оптимизации никаким образом не повлияли на данный режим ввиду полного отсутствия алгоритмов.
3. Против компьютера -- Основной режим данный игры, включает три уровня сложности:
    - Легкий(подбирается любое слово до 3 букв);
    - Средний(подбираются слова, приемущественно от 4 до 5 букв);
    - Сложный(подбираются максимально длинные слова).

Игра заканчивается тогда, когда либо заполнены все клетки, либо невозможно составить очередное слово согласно указанным выше правилам.
## Основные технические особенности
Чтобы обеспечить наиболее оптимальный и быстрый перебор слов, в качестве структуры данных для хранения было выбрано дерево, в узлах которого хранятся слова целиком.

Далее, для ускорения, помимо "словарного" дерева было создано инвертированное префиксное дерево.
> Префикс -- структура данных реализующая интерфейс ассоциативного массива, то есть позволяющая хранить пары «ключ-значение».

В данном случае, в качестве ключей выступают строки. На листьях этого дерева располагаются инвертированные слова. Данная оптимизация необходима из-за особенностей правил игры: Буква, поставленная в клетку может стоять на любом месте слова.

Таким образом, поиск слова разделен на поиск по префиксному дереву, а затем по словарному. Это ускоряет все три сложности режима игры против компьютера. 
### Локальные оптимизации
В связи с тем, что полный перебор необходим только в одном случае, можно оптимизировать поиск, путем раннего выхода. Так, на этапе перебора смежных клеток, то есть на самом верхнем уровне работы алгоритма, можно выходить при текущей длине слова равной или большей 3 (легкая сложность), 4 (средняя сложность) и 24 (высокая сложность, самое длинное слово в словаре). 
Спускаясь на уровни ниже были добавлены схожие проверки, путем установления флагов.
Всего досрочный выход может случиться на этапах:
- Перебор смежных клеток
- Перебор букв в одной клетке
- Поиск слова в инвертированном дереве
- Поиск слова в словрном дереве

## Итог
Оценка оптимизаций на разных этапах производилась путем сравнение графиков зависимостей свободных клеток ко времени, затраченного на постановку слова.

### Версия первая
Данная версия не имеет никаких оптимизаций, работает путем поиска лишь по одному дереву. Из-за наличия полного перебора на каждом ходе, поиск слова занимает десятки секунд.
### Версия вторая
Появилось префиксное дерево, также имеем полный перебор, однако поиск осуществляется в разы быстрее, чем в пердыдущей версии.

![alt text](https://github.com/powerfynger/balda_3_sem_orig/blob/master/images/var_2.png)
