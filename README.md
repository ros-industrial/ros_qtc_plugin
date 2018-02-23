# ROS Qt Creator Plug-in
This branch is an orphan used to store GitHub pages for the repository website.

## Update documentation
 - Install tool to generate documentation for multiple versions from [here](https://robpol86.github.io/sphinxcontrib-versioning/master/install.html).
 - Update documentation in each branch.
 - Push changes for each branch
 - Checkout the **master** branch
 - Build Documentation (Note: Change **4.4** to the latest version)
   - Navigate to the repository.
   - `sphinx-versioning build -i -b -B 4.4 -r 4.4 -w "^4.*" gh_pages .`
   - Option Info
     - `-i`:       Reverse the order of versions
     - `-b`:       Show a warning banner on version that are not the latest
     - `-B 4.4`:   Show a ref to 4.4 on the warning which is the latest version
     - `-r 4.4`:   Set the latest version to 4.4
     - `-w "^4.*"` Set the whitelist to only include branch names starting with 4.
     - `gh_pages`  Directory name containing the source. Must be named the same in all brachges
 - Push changes
