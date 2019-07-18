Setup Qt Creator for ROS
========================

Setup Ubuntu to allow debugging/ptrace
--------------------------------------

#. Open file: :code:`sudo gedit /etc/sysctl.d/10-ptrace.conf`
#. Change the value of :code:`kernel.yama.ptrace_scope` to 0
#. Reload the kernel configuration with :code:`sudo systemctl restart procps.service`

Set Theme
---------
If version 3.3.1 or higher was installed you are able to set the theme to dark following the steps bellow.

#. Open Qt Creator
#. Goto: :kbd:`Tools` > :kbd:`Options` > :kbd:`Environment` > :kbd:`General`
#. There should be a setting for Theme in the "User Interface" group containing a drop down box with two options "default or dark".

Set Syntax Color Schemes
----------------------------

#. Open Qt Creator
#. Goto: :kbd:`Tools` > :kbd:`Options` > :kbd:`Text Editor` > :kbd:`Font & Colors`
#. There is a drop down box in the "Color Scheme" group where you select different syntax color schemes.
#. Addition schemes can be added as explained in the below links.

   #. https://github.com/welkineins/qtcreator-themes
   #. https://github.com/alexpana/qt-creator-wombat-theme

Set ROS Code Format
-------------------

#. Open Qt Creator
#. On the sidebar: :kbd:`Projects` > :kbd:`Editor`

   * Changing it globally at :kbd:`Tools` > :kbd:`Options` > :kbd:`C++` or within :kbd:`Projects` > :kbd:`Code Style` does not work.

Setup Clang Formatting
----------------------

#. Install Clang `sudo apt-get install clang-format-6.0`
#. Goto: :kbd:`Tools` > :kbd:`Options` > :kbd:`Environment` > :kbd:`External Tools`
#. Select: :kbd:`Add` > :kbd:`Add Tool`
#. Fill in the information below.

   * Description: Clang Cpp Format
   * Executable: /usr/bin/clang-format-6.0
   * Arguments::

     -style="{Language: Cpp, AccessModifierOffset: -2, AlignAfterOpenBracket: true, AlignEscapedNewlinesLeft: false, AlignOperands:   true, AlignTrailingComments: true, AllowAllParametersOfDeclarationOnNextLine: true, AllowShortBlocksOnASingleLine: false, AllowShortCaseLabelsOnASingleLine: false, AllowShortIfStatementsOnASingleLine: false, AllowShortLoopsOnASingleLine: false, AllowShortFunctionsOnASingleLine: All, AlwaysBreakAfterDefinitionReturnType: false, AlwaysBreakTemplateDeclarations: false, AlwaysBreakBeforeMultilineStrings: false, BreakBeforeBinaryOperators: None, BreakBeforeTernaryOperators: true, BreakConstructorInitializersBeforeComma: false, BinPackParameters: true, BinPackArguments: true, ColumnLimit:     80, ConstructorInitializerAllOnOneLineOrOnePerLine: false, ConstructorInitializerIndentWidth: 4, DerivePointerAlignment: false, ExperimentalAutoDetectBinPacking: false, IndentCaseLabels: false, IndentWrappedFunctionNames: false, IndentFunctionDeclarationAfterType: false, MaxEmptyLinesToKeep: 1, KeepEmptyLinesAtTheStartOfBlocks: true, NamespaceIndentation: None, ObjCBlockIndentWidth: 2, ObjCSpaceAfterProperty: false, ObjCSpaceBeforeProtocolList: true, PenaltyBreakBeforeFirstCallParameter: 19, PenaltyBreakComment: 300, PenaltyBreakString: 1000, PenaltyBreakFirstLessLess: 120, PenaltyExcessCharacter: 1000000, PenaltyReturnTypeOnItsOwnLine: 60, PointerAlignment: Left , SpacesBeforeTrailingComments: 1, Cpp11BracedListStyle: true, Standard: Cpp11, IndentWidth: 2, TabWidth: 8, UseTab: Never, BreakBeforeBraces: Allman, SpacesInParentheses: false, SpacesInSquareBrackets: false, SpacesInAngles:  false, SpaceInEmptyParentheses: false, SpacesInCStyleCastParentheses: false, SpaceAfterCStyleCast: false, SpacesInContainerLiterals: true, SpaceBeforeAssignmentOperators: true, ContinuationIndentWidth: 4, CommentPragmas:  '^ IWYU pragma:', ForEachMacros:   [ foreach, Q_FOREACH, BOOST_FOREACH ], SpaceBeforeParens: ControlStatements, DisableFormat:   false}" -i %{CurrentDocument:FilePath}
   * Working directory: %{CurrentProject:Path}
   * Output: Show in Pane
   * Error output: Show in Pane
   * Environment: No Changes to apply.
   * Modifies current document: Checked

#. Select :kbd:`Apply`
#. Now lets add a quick key.
#. Goto: :kbd:`Tools` > :kbd:`Options` > :kbd:`Environment` > :kbd:`Keyboard`
#. In the filter box type "Clang" and you should pull up the new tool.
#. In the Shortcut section enter the Target text box and press :kbd:`Ctrl + Shift + k` to set the shortcut.
#. Now to apply the Clang format to a C++ file open the file in Qt Creator and press :kbd:`Ctrl + Shift + k` and the file should be formatted correctly.

Preventing Qt Creator form stepping into Boost, Eigen, etc.
-----------------------------------------------------------

#. First clone this repository https://github.com/Levi-Armstrong/gdb-7.7.1.git
#. Follow the instruction in the README file

   #. ./configure
   #. make
   #. sudo checkinstall

#. Goto: :kbd:`Tools` > :kbd:`Options` > :kbd:`Debugger` > :kbd:`GDB`
#. Add the following code below to the "Additional Startup Commands"

   .. code-block:: python

      skip pending on
      python
      for root, dirs, files in os.walk("/usr/include/boost/"):
        for file in files:
          if file.endswith(".hpp"):
            cmd = "skip file " + os.path.join(root, file)
            gdb.execute(cmd, True)

      for root, dirs, files in os.walk("/usr/include/eigen3/Eigen/"):
        for file in files:
          if file.endswith(".hpp"):
            cmd = "skip file " + os.path.join(root, file)
            gdb.execute(cmd, True)
      end
      skip enable

#. Now when you are stepping through your code it should not step into Boost or Eigen. You can also add additional directories following the same process.
#. Also if you would like to skip a particular function refer to the GDB documentation for instruction. It is something along the lines of `skip function function_name`.
