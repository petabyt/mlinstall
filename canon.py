import json

# This list may be old and inaccurate
models = [
    ["EOS 100D", "REBEL SL1", "Kiss X7"],
    ["EOS 200D", "REBEL SL2", "Kiss X9"],
    ["EOS 500D", "REBEL T1i", "Kiss X3"],
    ["EOS 550D", "REBEL T2i", "Kiss X4"],
    ["EOS 600D", "REBEL T3i", "Kiss X5"],
    ["EOS 650D", "REBEL T4i", "Kiss X6i"],
    ["EOS 700D", "REBEL T5i", "Kiss X7i"],
    ["EOS 750D", "REBEL T6i", "Kiss X8i"],
    ["EOS 760D", "REBEL T6s", "8000D"],
    ["EOS 800D", "REBEL T7i", "Kiss X9i"],
    ["EOS 1100D", "REBEL T3", "Kiss X50"],
    ["EOS 1200D", "REBEL T5", "Kiss X70"],
    ["EOS 1300D", "REBEL T6", "Kiss X80"],
    ["EOS 2000D", "REBEL T7", "Kiss X90"],
    ["EOS 4000D", "REBEL T100", "3000D"],
    ["EOS 50D"],
    ["EOS 60D"],
    ["EOS 60Da"],
    ["EOS 70D"],
    ["EOS 80D"],
    ["EOS 6D"],
    ["EOS 6D Mark II"],
    ["EOS 7D"],
    ["EOS 7D Mark II"],
    ["EOS 77D", "9000D"],
    ["EOS 5D Mark II"],
    ["EOS 5D Mark III"],
    ["EOS 5D Mark IV"],
    ["EOS 5Ds"],
    ["EOS 5Ds R"],
    ["EOS-1D Mark IV"],
    ["EOS-1D C"],
    ["EOS-1D X"],
    ["EOS-1D X Mark II"]
]

# Verify a camera is Canon and supported
# By the via input
def isCanon(string):
    for cam in models:
        for alt in cam:
            if alt.lower() in string.lower():
                return True
    return False

downloads = {
    "EOS 1300D": ["1.1.0", "https://bitbucket.org/ccritix/magic-lantern-git/downloads/magiclantern-Nightly.2021Jun15.1300D110.zip"],
    "EOS 5D Mark III": ["1.1.3", "https://builds.magiclantern.fm/jenkins/job/5D3.113/468/artifact/platform/5D3.113/magiclantern-Nightly.2018Jul03.5D3113.zip"]
}

def getML(string, version):
    name = ""
    for cam in models:
        for alt in cam:
            if alt.lower() in string.lower():
                if downloads[cam[0]][0] == version:
                    return {
                        "name": cam[0],
                        "ml": downloads[cam[0]][1]
                    }
    return False