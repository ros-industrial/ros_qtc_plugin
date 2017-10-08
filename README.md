# ROS Qt Creator Plug-in
This branch is an orphan used to store GitHub pages for the repository website.

## Update documentation
 - Install tool to generate documentation for multiple versions from [here](https://robpol86.github.io/sphinxcontrib-versioning/master/install.html).
 - Update documentation in each branch.
 - Push changes for each branch
 - Checkout the **master** branch
 - Build Documentation (Note: Change **4.4** to the latest version)
   - Navigate to the repository.
   - `sphinx-versioning build -i -b -B 4.4 -r 4.4 gh_pages .`
 - Push changes
