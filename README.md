# What and Why Blockchain?

블록체인을 직접 설계·구현하면서 **“왜 블록체인이 가치가 있는 화폐가 될 수 있는가?”** 를 이해하는 프로젝트입니다.
백엔드(C++17) + 프런트(React)로 구성되며, 두 노드 간 P2P 전파를 시각적으로 확인할 수 있도록 구현했습니다.
직접 만들어보며 블록체인이 어떻게 구성되고, 왜 탈중앙이 가치가 생기는지를 경험하는 과정입니다.

## 목표

- 블록·트랜잭션·UTXO 구조를 직접 다뤄보며 블록체인의 핵심 개념 이해
- 두 노드(8080/8081) 사이 블록 전파를 통해 “탈중앙 동기화”를 체험
- 왜 블록체인이 주목받는지(검열 저항, 중단 저항, 신뢰 최소화 등) 감 잡기

## 실행 화면

📌 프런트엔드 메인 화면
(트랜잭션 생성 / 블록 상태 확인)

[프론트 메인](기본%20화면.pngpng)

📌 채굴 난이도 설정

[채굴 난이도 설정](난이도%20설정.pngpng)

📌 블록 채굴 시

[채굴 버튼 클릭](마이닝%20시.pngpng)

📌 블록 채굴 후

[채굴 완료](마이닝%20완료.png)
[블록 추가](마이닝%20후%20블록%20추가.png)
[8080서버](마이닝%20후%208080%20사바.png)
📌 트랜잭션 추가
[거래 추가 전](거래%20추가%20전.pngpng)

📌 트랜잭션 추가 후 및 블록 채굴
[거래 추가](트랜잭션%20추가%20시.pngpng)
[마이닝](트랜잭션%20추가%20후%20마이닝.pngpng)
📌 두 노드 실행 모습

(8080 / 8081 로그 비교)
[8080서버로그](8080서버%20로그.png)
[8081 로그 명령](8081%20블록%20보는%20명령.png)
[8081 로그](8081%20노드%20블록%20보기.png)
[여기에 GIF 또는 이미지 추가]
docs/screenshots/two-nodes-running.gif

## 실행 방법

필수: CMake, Node/NPM  
데이터 초기화(선택): `rm toychain/data/chain.db toychain/data/chain.dat`

1. 백엔드 빌드  
   `cd toychain/backend && cmake --build build`
2. 노드 실행(터미널 2개)
   - `PORT=8080 PEERS=http://localhost:8081 ./build/toychain_server`
   - `PORT=8081 PEERS=http://localhost:8080 ./build/toychain_server`
3. 프런트 실행  
   `cd toychain/frontend && npm install && npm run dev`  
   기본 API_BASE는 8080입니다. 8081을 보고 싶으면 `src/App.jsx`의 `API_BASE`를 8081로 바꾼 프런트를 따로 띄우세요.

## 데모 시나리오 (녹화용)

1. 프런트에서 트랜잭션 추가 (예: alice→bob).
2. 8080 노드에서 Mine → 로그에 “Block mined” 출력.
3. `curl http://localhost:8081/blockchain` 혹은 8081을 가리키는 프런트 탭에서 새 블록 반영 확인.
4. 8081에서 Mine → 8080에도 반영되는지 확인.
5. 스크린샷/GIF 위치 예: `docs/screenshots/node-sync.png`, `docs/screenshots/frontend.png` (후에 직접 캡처본 추가).

## 리셋 방법

- 두 노드 종료 후 `rm toychain/data/chain.db toychain/data/chain.dat`
- 다시 빌드/실행하면 제네시스부터 시작

## 주의 (trust 모드)

- 외부 블록/tx 해시 검증을 생략합니다. 전파 데모가 목적이며 보안/무결성 검증 용도가 아닙니다.
- 검증 모드로 바꾸려면: tx/block 직렬화·해시를 양 노드가 동일하게 맞춘 뒤 해시 검증을 켜고 **새 체인으로 시작**해야 합니다.
