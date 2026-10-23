# Reverie Ensemble 0.1.0

QuadraJ의 무료 오픈소스 네이티브 DAW. C++20으로 작성한 **Reverie Audio Core**와 Qt 6 Widgets를 사용합니다. 계정·클라우드·런타임 네트워크 연결 없이 MIDI 작곡, 멀티트랙 재생과 믹싱, WAV 가져오기·녹음, 프로젝트 저장 및 오프라인 WAV 출력을 제공합니다.

자체 코드는 Apache License 2.0입니다. Qt와 제삼자 구성 요소는 각각의 라이선스를 유지합니다. [LICENSE](LICENSE), [NOTICE](NOTICE), [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md)를 읽어 주세요.

## macOS 빌드 및 실행

Xcode Command Line Tools 또는 Xcode와 동적 링크 Qt 6.4 이상이 필요합니다. 이 환경에서는 Apple Silicon, Xcode Clang 21, Homebrew Qt 6.11.2로 실제 빌드·창 실행·출력 장치 재생을 확인했습니다. Intel macOS / Windows / Linux 실행은 아직 검증하지 않았습니다.

```sh
xcode-select --install # Xcode/Command Line Tools가 없을 때만
brew install cmake ninja qtbase
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase)"
cmake --build build -j 4
ctest --test-dir build --output-on-failure
open build/ReverieEnsemble.app
```

이미 생성된 이 작업 폴더의 `build/ReverieEnsemble.app`도 바로 실행할 수 있습니다. 개발용 앱은 설치된 Qt 동적 라이브러리를 사용하므로 Homebrew Qt를 삭제하면 다시 구성해야 합니다. 배포용 서명·공증·Qt 라이브러리 수집은 [배포 문서](docs/DISTRIBUTION.md)를 따릅니다. 현재 빌드 자체를 독립 배포용 패키지로 표기하지 않습니다.

Release 빌드:

```sh
cmake -S . -B build-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$(brew --prefix qtbase)"
cmake --build build-release -j 4
ctest --test-dir build-release --output-on-failure
open build-release/ReverieEnsemble.app
```

오디오 장치 없는 환경에서도 GUI와 렌더러를 사용할 수 있습니다.

```sh
build/ReverieEnsemble.app/Contents/MacOS/ReverieEnsemble --no-audio
build/reverie-render --demo out.wav
build/reverie-render song.reverie song.wav
```

Qt 없이 코어만 빌드할 수도 있습니다.

```sh
cmake -S . -B build-core -G Ninja -DREVERIE_BUILD_GUI=OFF
cmake --build build-core -j 4
ctest --test-dir build-core --output-on-failure
```

## 첫 곡 만들기

1. 앱은 두 MIDI 트랙으로 구성된 직접 제작 데모 `First light`로 시작합니다. **Play / Space**로 실제 소리를 재생합니다. **File → New**는 빈 프로젝트입니다.
2. **+ MIDI track**을 누르고 트랙 빈 영역을 더블클릭하면 4박 MIDI 클립이 생깁니다. 클립을 더블클릭하면 하단 피아노 롤에서 편집합니다.
3. 피아노 그리드를 클릭해 노트를 추가합니다. 노트 몸체를 드래그하면 위치·높이, 오른쪽 끝을 드래그하면 길이가 바뀝니다. Cmd/Ctrl 클릭으로 복수 선택, Shift 드래그로 영역 선택, 우클릭 또는 Delete로 삭제합니다. Velocity 입력은 선택된 노트에 적용됩니다. **Quantize / Q**는 선택된 노트 또는 선택이 없으면 클립 전체에 적용합니다.
4. 왼쪽 악기 이름을 더블클릭하거나 오른쪽 프리셋 선택 상자를 사용합니다. 8종 신스 프리셋과 합성 드럼을 제공합니다. 실제 피아노·오르간 녹음을 재현한 샘플 악기로 광고하지 않습니다. 드럼 MIDI 키는 36 kick, 38 snare, 42 hi-hat, 46 open hat입니다.
5. 트랙을 선택하면 오른쪽에서 Volume / Pan / Mute / Solo와 악기 ADSR·오실레이터·필터를 조정할 수 있습니다. Master는 전체 출력입니다. 미터는 실제 DSP peak이며, 클리핑 경고는 **Reconnect audio / clear clip**으로 지웁니다.
6. 클립 몸체를 드래그해 이동하고, 오른쪽 끝을 드래그해 길이를 바꿉니다. 같은 종류 트랙 사이로 이동할 수 있습니다. Cmd/Ctrl+D는 선택 클립을 뒤에 복제합니다. 우클릭 메뉴에서 이름·트랙 색상·순서·삭제를 조절합니다. 트랙 이름은 헤더 더블클릭으로 변경합니다.
7. **File → Save**는 `.reverie` JSON과 옆의 `<프로젝트명>.media/` 폴더에 소스 WAV를 수집합니다. 두 항목을 함께 옮기면 됩니다. **Save As**는 새 미디어 폴더를 만듭니다. **File → Open**으로 복원합니다.
8. **File → Export WAV**는 48 kHz / stereo / 16-bit PCM WAV를 만듭니다. 끝에 최소 2초, 더 긴 release 설정이 있으면 그만큼의 여유를 둡니다. 재생과 같은 DSP를 사용하며 오디오 장치가 없어도 출력됩니다.

## WAV, 녹음, 샘플러

- **Import WAV…**는 PCM / IEEE-float RIFF WAV를 디코딩하고 파형 캐시를 백그라운드에서 만듭니다. 오디오 트랙 선택 시 그 트랙에, MIDI 트랙 선택 시 새 오디오 트랙에 배치합니다. 리샘플링은 선형 보간입니다. 클립 길이 변경은 원본 파일을 수정하지 않습니다. MIDI/오디오 트랙의 Undo / Redo는 프로젝트 스냅샷을 사용하며 PCM을 매번 복제하지 않습니다.
- 녹음 전에 프로젝트를 저장하고 오디오 트랙과 입력을 선택합니다. **Choose / refresh inputs**에서 장치를 찾고 **Record**를 누릅니다. macOS 마이크 권한은 이때 요청합니다. **Stop / Finish take**는 디스크 작업 스레드를 마무리하고 WAV 클립을 녹음 시작 위치에 넣습니다. 권한·장치·디스크 실패는 앱 종료 대신 오류로 처리합니다. 녹음 중에는 편집을 잠급니다.
- **Sampler WAV…**는 선택한 MIDI 트랙에 사용자 WAV를 음높이에 따라 재생하는 악기를 연결합니다. 루트는 MIDI 60(C4)입니다. 프리셋 JSON과 WAV는 별도 파일이며 출처·라이선스 메타데이터를 보관합니다. 외부 WAV의 재배포 권한은 사용자가 확인해야 합니다. 자동 다운로드는 없습니다.
- **Save / Load preset**은 신스 파라미터와 샘플 참조를 JSON으로 저장·복원합니다.

## 구현 상태와 제한

작곡 → 실제 장치 재생 → 다중 트랙 믹싱 → 저장·복원 → WAV 출력 경로와 WAV 가져오기·녹음 작업 스레드를 구현했습니다. 타임라인·피아노 롤·미터는 실제 엔진과 모델에 연결되어 있습니다. 구현과 검증의 차이는 [테스트 기록](docs/TESTING.md)에 구분했습니다.

0.1의 범위:

- 기본 48 kHz / 256 frame / float32 처리. 코어와 테스트는 44.1 kHz도 지원합니다. 장치 내부 변환은 miniaudio에 맡깁니다. GUI에서 샘플레이트·버퍼 크기 변경은 아직 제공하지 않습니다.
- 최대 64트랙, 신스 트랙당 32보이스. 글로벌 고정 템포와 기본 박자표를 지원하며 템포 자동화는 없습니다.
- WAV당 최대 20분 또는 디코딩된 stereo float PCM 256 MiB. 프로젝트 로드 시 미디어 캐시 예산 512 MiB를 넘는 추가 파일은 경고와 함께 누락 미디어로 처리합니다. 녹음은 디스크로 스트리밍하지만 현재 편집기로 다시 읽을 수 있는 크기 내에서 사용해야 합니다.
- 재생 중 볼륨·팬·Mute·Solo 변경은 보이스 상태 유지와 128샘플 게인 램프를 사용합니다. 악기·노트 스케줄 변경이나 seek는 유지 중 노트의 envelope를 다시 시작합니다. 완전한 sample phase 복원은 아닙니다. Master 변경은 현재 즉시 적용됩니다.
- 클리핑은 경고하고 최종 출력은 `[-1, 1]`로 제한합니다. 전문 limiter, dither, true-peak 측정은 없습니다.
- 오디오 클립은 소스 offset을 모델에 보관하고 오른쪽 trim을 UI에서 지원합니다. 왼쪽 trim·split·fade·warping·time stretch는 아직 없습니다. 겹친 오디오 클립은 합산합니다.
- 피아노 롤 화면 범위는 MIDI 24–95이며 모델은 0–127입니다. 외부 MIDI 키보드·MIDI 파일 입출력·노트 오디션·루프·메트로놈은 아직 없습니다.
- 입력은 별도 capture 장치입니다. 입력 monitoring, 녹음 지연 보정, 서로 다른 장치의 clock drift 보정, punch-in은 없습니다. 실제 마이크 녹음·거부 흐름·긴 녹음·장치 분리·장시간 글리치는 수동 검증이 필요합니다.
- 저장은 같은 디렉터리의 임시 JSON을 flush/close 후 rename하는 방식입니다. macOS에서 기존 파일을 원자적으로 교체하며 OS 전원 장애 내구성(fsync)은 별도 작업입니다. Windows 기존 파일 교체·유니코드 미디어 경로는 아직 검증하지 않았습니다.
- SF2 / FluidSynth, 플러그인 호스팅, 고급 이펙트, 대규모 라이브러리는 포함하지 않았습니다. SF2 검토 결과와 우선순위는 [0.2 로드맵](docs/ROADMAP.md)에 있습니다.
- 독립 배포 패키지·앱 아이콘 세트·Developer ID 서명·공증·Intel universal 바이너리는 아직 제공하지 않습니다.

## 구조와 개발

- `src/core/`: Qt 없는 데이터 모델, 신스, 샘플 스케줄링, 그래프, 녹음, 미디어, JSON 직렬화
- `src/platform/`: miniaudio 장치, macOS 권한과 앱 번들 메타데이터
- `src/ui/`: Qt Widgets 편집기, 커스텀 QPainter 타임라인·피아노 롤
- `tests/`: 하드웨어 없는 DSP/저장/녹음/동시성 테스트와 Qt 편집 워크플로 테스트
- `third_party/`: 고정 버전 miniaudio와 nlohmann/json, 라이선스 원문

[오디오 엔진 설계](docs/ARCHITECTURE.md), [프로젝트 포맷](docs/PROJECT_FORMAT.md), [기여 방법](CONTRIBUTING.md).
