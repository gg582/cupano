# cupano (한국어)

[English README](README.md)

cupano는 같은 원두에 대한 리뷰를 한국, 일본, 대만, 영어권, 노르웨이, 독일 여섯 커피 문화권에서 모아 비교하는 Chrome·Firefox 확장 프로그램입니다. 여러 지역이 공통으로 꼽는 테이스팅 노트와 특정 지역이나 개인에게서만 나온 노트를 구분해 보여줍니다. 여섯 개 언어를 읽는 사람은 드물어서 커피 리뷰는 지역 안에 갇혀 있기 쉬운데, cupano가 문화권을 넘나들며 대신 읽어줍니다.

## 동작 방식

원두 상품 페이지를 열거나 구글, 네이버 같은 검색 엔진에서 원두를 검색한 뒤 cupano 아이콘을 누르고 분석을 누르세요. cupano가 페이지를 읽고, 선택한 AI 제공자에게 지역 에이전트 여섯 개를 돌려 각 언어로 검색하게 합니다. 돌아온 결과는 C++로 작성된 WebAssembly 코어가 처리합니다. 지역별로 노트가 얼마나 자주 나왔는지 세고, SCA / WCR 커피 테이스터 향미 휠 위에 노트를 배치하고, 로스터리마다 다른 로스팅 표현을 하나의 기준으로 맞추고, 전 세계와 내 나라에서 품절인지 확인합니다. 좋아하는 맛을 적어두면 이 원두가 취향에 얼마나 맞는지도 점수로 알려줍니다.

분석은 백그라운드에서 진행되기 때문에 팝업을 닫았다가 나중에 다시 열어 결과를 볼 수 있습니다. 화면은 한국어, 영어, 일본어, 번체 중국어, 노르웨이어, 독일어를 지원합니다.

## API 키

본인의 Google Gemini 또는 OpenAI API 키가 필요합니다. Gemini 키는 https://aistudio.google.com/api-keys 에서 무료로 만들 수 있고, 팝업의 ⚙ 버튼을 누른 뒤 Advanced에서 입력하면 됩니다.

## 개인정보

cupano에는 자체 서버가 없습니다. 페이지 글자는 브라우저에서 선택한 AI 제공자로 바로 전송되고, 국가를 직접 고정하지 않으면 IP로 국가를 조회합니다. API 키는 브라우저 안에만 저장됩니다. 자세한 내용은 [PRIVACY.md](PRIVACY.md)에 있습니다.

## 소스에서 빌드하기

Emscripten 5.0.0 이상, Node.js와 npm, GNU Make, zip이 필요합니다.

```sh
source /path/to/emsdk/emsdk_env.sh
npm ci
make            # src/cpp를 wasm/으로 빌드하고 폴리필을 vendor/로 복사
```

Chrome에서는 `chrome://extensions`에서 개발자 모드를 켜고 프로젝트 폴더를 압축 해제된 확장 프로그램으로 불러오면 되고, Firefox에서는 `about:debugging`에서 불러오면 됩니다.

스토어 제출용 패키지는 `./publish.sh`로 만듭니다. Chrome용은 `publish/chrome/`에, Firefox용과 AMO 심사에 필요한 소스 압축 파일은 `publish/firefox/`에 생성됩니다.

## 라이선스

GPL v2. [LICENSE](LICENSE)를 참고하세요.
