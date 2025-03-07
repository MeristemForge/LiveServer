import { hilog } from '@kit.PerformanceAnalysisKit';
import testNapi from 'libentry.so';
import { abilityAccessCtrl, common, Permissions } from '@kit.AbilityKit';
import { BusinessError } from '@kit.BasicServicesKit';

@Entry
@Component
struct Index {
  @State message: string = 'Hello World';

  build() {
    Row() {
      Column() {
        Button("START")
          .onClick(()=>{
            testNapi.liveServerStart();
          })
        Button("STOP")
          .onClick(()=>{
            testNapi.liveServerStop();
          })
      }
      .width('100%')
    }
    .height('100%')
  }
  aboutToAppear() {
    hilog.info(0x0000, 'LiveServer', 'aboutToAppear.....................');
    const context: common.UIAbilityContext = getContext(this) as common.UIAbilityContext;
    reqPermissionsFromUser( ['ohos.permission.CAMERA'], context);
  }
}

function reqPermissionsFromUser(permissions: Array<Permissions>, context: common.UIAbilityContext): void {
  let atManager: abilityAccessCtrl.AtManager = abilityAccessCtrl.createAtManager();
  atManager.requestPermissionsFromUser(context, permissions).then((data) => {
    let grantStatus: Array<number> = data.authResults;
    let length: number = grantStatus.length;
    for (let i = 0; i < length; i++) {
      if (grantStatus[i] === 0) {
        hilog.info(0x0000, 'LiveServer', 'permission success.....................');
      } else {
        hilog.info(0x0000, 'LiveServer', 'permission failure.....................');
        return;
      }
    }
  }).catch((err: BusinessError) => {
    hilog.info(0x0000, 'LiveServer', 'panic.....................');
  })
}