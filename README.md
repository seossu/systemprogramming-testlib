# testlib — Pintos 자료구조(list · hash · bitmap) 테스트 프로그램

Pintos 커널의 범용 자료구조 라이브러리(이중 연결 리스트, 해시 테이블, 비트맵)를 사용자 프로그램으로 옮기고,
명령어를 한 줄씩 입력받아 각 자료구조를 만들고 조작하는 대화형 테스트 프로그램입니다. (시스템프로그래밍 과제)

## 빌드와 실행

```bash
make            # → ./testlib
./testlib < input.txt
```

표준 입력으로 명령을 한 줄씩 받고, `quit`을 만나면 종료합니다.

```
$ printf 'create list L\nlist_push_back L 3\nlist_push_back L 1\nlist_push_back L 2\nlist_sort L\ndumpdata L\nquit\n' | ./testlib
1 2 3
```

## 명령

| 분류 | 명령 |
|---|---|
| 공통 | `create list <이름>` · `create hashtable <이름>` · `create bitmap <이름> <비트 수>` · `delete <이름>` · `dumpdata <이름>` · `quit` |
| list | `push_front/back` `pop_front/back` `front` `back` `insert` `insert_ordered` `remove` `splice` `swap` `sort` `unique` `reverse` `shuffle` `max` `min` `size` `empty` |
| hash | `insert` `find` `delete` `replace` `apply <square\|triple>` `size` `empty` `clear` |
| bitmap | `mark` `reset` `flip` `test` `set` `set_all` `set_multiple` `count` `contains` `any` `none` `all` `scan` `scan_and_flip` `size` `dump` `expand` |

목록·해시·비트맵 명령은 각각 `list_`, `hash_`, `bitmap_` 접두어를 붙여 씁니다 (예: `list_sort L`, `hash_apply H square`, `bitmap_mark B 40`).
자료구조는 이름으로 구분하며 최대 10개까지 동시에 만들 수 있습니다.

## 구현

- **main.c**: 명령 파싱과 실행. 리스트·해시·비트맵을 하나의 `union`으로 묶어 이름으로 관리합니다.
- **Pintos 라이브러리에 추가한 함수**
  - `list_swap`, `list_shuffle` (list.c)
  - `hash_int_2`: 정수용 두 번째 해시 함수 (hash.c)
  - `bitmap_expand`: 기존 비트를 유지한 채 비트맵 크기를 늘림 (bitmap.c)
- 리스트와 해시 원소는 Pintos 방식대로 `list_elem`, `hash_elem`을 구조체 안에 넣고 `list_entry`/`hash_entry`로 꺼냅니다.

## 64비트 비트맵 버그 수정

Pintos 원본 `bitmap.c`는 `bitmap_mark`·`bitmap_reset`·`bitmap_flip`에 x86 32비트 인라인 어셈블리(`orl %k`, `andl %k`, `xorl %k`)를 씁니다.
비트를 담는 단위인 `unsigned long`이 64비트인 환경에서는 이 명령이 하위 32비트만 다뤄서, **각 64비트 단위의 32~63번 비트를 조작할 수 없었습니다.**
같은 의미의 C 연산(`|=`, `&= ~`, `^=`)으로 바꿔서 모든 비트가 정상적으로 동작하도록 했습니다.

```
create bitmap B 64 → bitmap_mark B 40 → bitmap_test B 40
수정 전:  false     수정 후:  true
```
