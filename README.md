# How to Run
1. 
```
git clone --depth=1 -b rpi-6.1.y https://github.com/raspberrypi/linux
```
2. code 디렉터리 -> Makefile <KDIR> 수정

3. make로 compile하여 .ko 파일 생성

4. 디바이스 파일 생성
```
sudo mknod /dev/keyled_dev c 230 0
```
5. 모듈 적재
```
sudo insmod ledkey_dev_pjb.ko
```
6. app.c 실행
```
sudo ./ledkey_app_pjb [led_val(0x00~0xff)] [timer_val(1/100)]
```
7. 적재된 모듈 해제
```
sudo rmmod ledkey_dev_pjb
```

# Device driver 구현
입출력 다중화와 블록킹 I/O를 활용한 디바이스 드라이버 구현

1. 구현목표

디바이스 드라이버는 입출력 다중화 (Poll), Blocking I/O를 구현하여 처리할 데이터가 없을 시 
프로세스를 대기 상태로 전환하고 key 인터럽트 발생시 wake up 하여 준비 / 실행 상태로 전환하여 처리

* 타이머 시간과 LED 값은 APP 실행시 Argument로 전달하여 처리

* 하드웨어 제어와 상태 정보를 얻기 위해 ioctl 함수 사용하여 명령어를 구현

2. 구현과정

ioctl 함수를 위한 헤더파일 작성

<그림>
 - TIMER_STATRT : 커널 타이머 시작
 - TIMER_STOP : 커널 타이머 정지
 - TIMER_VALUE : led on /off 주기

파일동작 구조체

<그림>

문자 디바이스 드라이버와 응용 프로그램을 연결하기 위해서 파일 오퍼레이션 구조체를 사용해야 한다. 구조체를 선언하면 함수의 주소를 구조체 필드에 대입하고 커널에 등록한다.

키 값에 따른 동작 구분
| key  | key 값에 해당하는 기능                                                                    |
|-------|-----------------------------------------------------------------------------------|
| 1     | 타이머를 정지 (TIMER_STOP)                                                              |
| 2     | 키보드롤 커널 타이머 시간을 100분의 1초 단위로 입력 받고 시간 변경 (TIMER_VALUE)                            |
| 3     | LED 값을 입력 받아 받은 값으로 on / off (0x00 ~ 0xff) 및 ‘Q’ 또는 ‘q’ 입력 시  타이머는 멈추고 응용 프로세스 종료 |
| 4     | 커널 타이머를 동작 (TIMER_START)                                                          |
| 8     | App 종료                                                                            |

3. 구현 결과

<그림>