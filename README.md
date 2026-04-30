# 🏗️ 스마트 물류 자동 분류 시스템 (Smart Sorting System)

> **로봇팔, 로드셀, 컨베이어 벨트를 결합한 무게 기반 자동화 제어 프로젝트**

---

## 📝 프로젝트 개요 (Overview)
로드셀 센서를 통해 물체의 무게를 실시간으로 측정하고, 데이터 값에 따라 컨베이어 벨트 구동과 로봇팔의 피킹(Picking) 동작을 유기적으로 제어하는 통합 자동화 솔루션입니다. 

단순한 하드웨어 구동을 넘어, 센서 데이터의 신뢰성을 높이기 위한 필터링 알고리즘 적용과 팀 단위의 효율적인 형상 관리에 집중했습니다.

## 🛠 기술 스택 (Tech Stack)
![C](https://img.shields.io/badge/C-A8B9CC?style=flat-square&logo=c&logoColor=white)
![C++](https://img.shields.io/badge/C++-00599C?style=flat-square&logo=cplusplus&logoColor=white)
![Git](https://img.shields.io/badge/Git-F05032?style=flat-square&logo=git&logoColor=white)
![GitHub](https://img.shields.io/badge/GitHub-181717?style=flat-square&logo=github&logoColor=white)
![STM32](https://img.shields.io/badge/STM32-03234B?style=flat-square&logo=stm32&logoColor=white) 

## ✨ 주요 기능 (Key Features)
* **정밀 무게 측정**: 로드셀 및 AD002 변환 모듈을 활용한 데이터 수집
* **신호 처리 알고리즘**: **이동 평균 필터(Moving Average Filter)**를 적용하여 센서 노이즈 최소화 및 데이터 정밀도 향상
* **실시간 통합 제어**: 무게 데이터에 따른 컨베이어 벨트 이송 및 로봇팔 분류 시퀀스 제어
* **협업 최적화**: 헤더(.h)와 소스(.c/.cpp) 파일 분리를 통한 모듈화 및 병합(Merge) 중심의 코드 관리

## 📂 시스템 아키텍처 (System Architecture)
1. **Sensing**: 로드셀을 통한 아날로그 신호 수집 및 디지털 변환
2. **Processing**: MCU 내 필터링 알고리즘 및 제어 로직 수행
3. **Acting**: 컨베이어 벨트 모터 및 6축 로봇팔 구동

## 👥 팀 협업 (Collaboration)
* **인원**: 6명
* **역할**: 시스템 로직 설계, 하드웨어 인터페이스 제어, 데이터 필터링 구현
* **관리**: GitHub를 통한 형상 관리 및 주기적인 코드 리뷰를 통한 안정성 확보

---
🔗 **상세 내용 확인**: [Project Notion Link](여기에_노션_링크를_넣어주세요)
