Modified to work on Linux Distro. Tested on ***Pop!\_OS 18.04 Lts***.  
The client can be built from the ![minetest_client](https://github.com/sunniel/minetest_client) project.  
The core components are the same as the original repo, excpet for **src/thread** folder, the files in which are replaced by [jthread](http://research.edm.uhasselt.be/jori/page/CS/Jthread.html) compiled with **Unix Makefiles** option in CMake  
## Dependency
- **include/nlohmann/** of [this repo](https://github.com/nlohmann/json.git)
- [boost](https://www.boost.org/)
- [Irrlicht](http://irrlicht.sourceforge.net/), install **libirrlicht-dev** by **apt**
## Additional setup
- go to **Properties > Makemake**, select **src:makemake** in main window, click **Options**
- select **Link**, click **More>>**, add **pthead** to "Additional libraries to link with: (-l option)" (to avoid build error with pthread)
- click **OK** and then click **Apply and Close**
