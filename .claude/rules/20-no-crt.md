# 20 — no-CRT C 제약 (MSVC / x64)

목표: C 런타임을 링크하지 않고 Win32 시스템 DLL만으로 동작하는 초소형 exe. → `10-size-budget.md`와 직결.

## 금지 (CRT 인입 유발)
- `malloc/free/calloc/realloc` → 정적 전역 풀 또는 `VirtualAlloc` 사용.
- `printf/sprintf/sscanf/puts` 등 stdio → 자작 정수→글리프 렌더.
- `sinf/cosf/sqrtf/powf` (libm) → SSE 인트린식(`_mm_sqrt_ss`) / 다항 근사 / 시작-시 테이블.
- `rand/srand` → `xorshift32` 자작.
- `memset/memcpy/memmove` 무분별 사용 → 필요 시 `#pragma function`으로 자작(재귀 인입 방지).
- C++ 예외/RTTI/STL/전역 생성자 → 사용 금지(C로 작성 권장).
- 파일 stdio(`fopen` 등) → Win32 `CreateFile`(스트레치: 최고기록 저장만).

## 필수 보일러플레이트
```c
int _fltused = 0;                 // float 사용 시 컴파일러가 참조
void WinMainCRTStartup(void){ /* ... */ ExitProcess(0); }   // 커스텀 엔트리
```
- 엔트리: `/ENTRY:WinMainCRTStartup`. 종료는 반드시 `ExitProcess`(return 금지).
- `hInstance`는 `GetModuleHandle(0)`.

## 링크 (명시 라이브러리만)
```
link /SUBSYSTEM:WINDOWS /ENTRY:WinMainCRTStartup /NODEFAULTLIB ^
     /OPT:REF /OPT:ICF kernel32.lib user32.lib gdi32.lib opengl32.lib winmm.lib
```
- `/NODEFAULTLIB`는 `#pragma comment(lib,...)`도 무시 → 라이브러리는 링크 커맨드에 직접 나열.

## 함정 체크리스트 (빌드 실패/크래시 예방)
- [ ] `_fltused` 정의했는가(float 쓰면 필수)
- [ ] 스택 지역배열 < 4KB (초과 시 `__chkstk` 인입) → 큰 배열은 전역/`VirtualAlloc`
- [ ] 큰 구조체 `{0}` 초기화가 `memset` 호출로 변환되는가 → 자작 memset 제공 or 필드 명시
- [ ] x64 타깃인가(`_ftol` 등 x86 헬퍼 회피)
- [ ] `/GS-`로 `__security_cookie` 제거했는가
- [ ] 큰 초기화 배열을 파일에 박지 않았는가(→ 시작 시 계산, 10-size §5)

## 권장 구조
- 엔티티는 전역 정적 풀 + active 플래그(힙 없음).
- 수학: `vec2` + 인트린식. RNG: 전역 `xorshift32`(QPC 시드).
- 오디오 링버퍼 등 대형 버퍼만 `VirtualAlloc`.
