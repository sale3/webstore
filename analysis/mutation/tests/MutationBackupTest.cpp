// Testovi za rezervnu kopiju i oporavak fajlova (saveBackup, recoverBackup).

#include "TestHelpers.h"

#include <stdexcept>
#include <string>

using namespace mutation;

TEST_F(TempProjectTest, SaveBackupCopiesFilesIntoBackupFolder)
{
    // Arrange
    writeFile("src/Cart.cpp", "original");
    const fs::path backupDir = buildDir_ / "mutation_backup";
    const FileSnapshot snapshot = takeSnapshot({root_ / "src" / "Cart.cpp"});

    // Act
    const bool saved = saveBackup(backupDir, root_, snapshot);

    // Assert
    EXPECT_TRUE(saved);
    EXPECT_EQ(readBytes(backupDir / "src" / "Cart.cpp"), "original");
}

TEST_F(TempProjectTest, SaveBackupReportsFailureWhenBackupCannotBeWritten)
{
    // Arrange
    // на мјесту гдје трега да буде копија налази се фолдер 
    writeFile("src/Cart.cpp", "original");
    const fs::path backupDir = buildDir_ / "mutation_backup";
    fs::create_directories(backupDir / "src" / "Cart.cpp");
    const FileSnapshot snapshot = takeSnapshot({root_ / "src" / "Cart.cpp"});

    // Act
    const bool saved = saveBackup(backupDir, root_, snapshot);

    // Assert
    EXPECT_FALSE(saved);
}

TEST_F(TempProjectTest, RecoverBackupThrowsAndKeepsBackupWhenFileCannotBeRestored)
{
    // Arrange
    // копија постоји, али је на месту фајла folder pa се фајл не може вратити
    const fs::path backupDir = buildDir_ / "mutation_backup";
    fs::create_directories(backupDir / "src");
    writeBytes(backupDir / "src" / "Cart.cpp", "original");
    fs::create_directories(root_ / "src" / "Cart.cpp");

    // Act & Assert
    EXPECT_THROW(recoverBackup(backupDir, root_), std::runtime_error);
    EXPECT_TRUE(fs::exists(backupDir));
}
